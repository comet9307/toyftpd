#include "download.h"
#include "select_cycle.h"
#include "block.h"
#include "virtual_file.h"
#include "tool.h"	
#include "protocal.h"

#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sys/socket.h>

#define BUF_SIZE 			8192

struct toy_download_pool_s toy_download_global_pool;

void toy_download_abort(struct toy_download_s *p_download);

int toy_download_add(u_int user_id, u_short *port_buf)
{
	struct toy_download_s *ptr = malloc(sizeof(struct toy_download_s));
	ptr->next = toy_download_global_pool.first_link;
	toy_download_global_pool.first_link = ptr;

	ptr->state = DS_LISTEN;
	ptr->fd = -1;
	ptr->user_id = user_id;

	ptr->filemsg = NULL;
	ptr->msg_len = 0;
	ptr->done_len = -1;

	ptr->md5_array = NULL;
	ptr->blocks_num = 0;
	ptr->file_size = 0;
	ptr->have_down_len = -1;

	ptr->data_buf = NULL;
	ptr->buf_size = 0;
	ptr->send_off = 0;
	++toy_download_global_pool.downloads_num;

	int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(listen_fd == -1)
	{
		toy_download_abort(ptr);
		return 0;
	}
	ptr->fd = listen_fd;
	toy_selcycle_fdset_add_fd(ptr->fd, FT_READ);

	listen(listen_fd, 1);
	struct sockaddr_in local_addr;
	socklen_t addr_len = sizeof(local_addr);
	getsockname(listen_fd, (struct sockaddr*)&local_addr, &addr_len);
	*port_buf = ntohs(local_addr.sin_port);
	return 1;
}

void toy_download_delete(struct toy_download_s *p_download)
{
	if(p_download == toy_download_global_pool.first_link)
		toy_download_global_pool.first_link = p_download->next;
	else
	{
		struct toy_download_s *ptr = toy_download_global_pool.first_link;
		while(ptr->next != p_download)
			ptr = ptr->next;
		ptr->next = p_download->next;
	}

	if(p_download->fd != -1)
	{
		if(toy_selcycle_isset(p_download->fd, FT_READ))
			toy_selcycle_fdset_rm_fd(p_download->fd, FT_READ);
		if(toy_selcycle_isset(p_download->fd, FT_WRITE))
			toy_selcycle_fdset_rm_fd(p_download->fd, FT_WRITE);
		close(p_download->fd);
	}

	if(p_download->filemsg)
		free(p_download->filemsg);
	if(p_download->md5_array)
		free(p_download->md5_array);
	if(p_download->data_buf)
		free(p_download->data_buf);

	--toy_download_global_pool.downloads_num;
}

void toy_download_accept(struct toy_download_s *p_download)
{
	struct sockaddr_in remote_addr;
	socklen_t addr_len;
	int fd = accept(p_download->fd, (struct sockaddr*)&remote_addr, &addr_len);
	toy_selcycle_fdset_rm_fd(p_download->fd, FT_READ);
	close(p_download->fd);
	if(fd == -1)
	{
		toy_upload_abort(p_download);
		return ;
	}
	toy_selcycle_fdset_add_fd(fd, FT_READ);
	p_download->fd = fd;
	p_download->state = DS_CONNECTED;
}

int toy_download_if_done(struct toy_download_s *p_download)
{
	if(p_download->have_down_len == p_download->file_size)
		return 1;
	return 0;
}


void toy_download_done(struct toy_download_s *p_download)
{
	toy_download_delete(p_download);
}

void toy_download_abort(struct toy_download_s *p_download)
{
	toy_download_delete(p_download);
}

void toy_download_read_from_block(struct toy_download_s *p_download)
{
	u_int cur_block = p_download->have_down_len / BLOCK_SIZE;
	u_int file_off = p_download->have_down_len - BLOCK_SIZE * cur_block;
	u_int block_size;
	if(cur_block == p_download->blocks_num - 1)
		block_size = p_download->file_size - BLOCK_SIZE * cur_block;
	else
		block_size = BLOCK_SIZE;
	u_int read_len;
	u_int buf_off;
	if(BUF_SIZE > block_size - file_off)
	{
		read_len = block_size - file_off;
		buf_off = BUF_SIZE - read_len;
	}
	else
	{
		read_len = BUF_SIZE;
		buf_off = 0;
	}
	toy_block_read(p_download->md5_array + cur_block * 16, p_download->data_buf + buf_off, file_off + sizeof(struct toy_block_header), read_len);
	p_download->send_off = buf_off;
}

void toy_download_recv_filemsg_finished(struct toy_download_s *p_download)
{
	char *filepath = p_download->filemsg;
	toy_vfile_get_md5(p_download->user_id, filepath, &p_download->md5_array, &p_download->blocks_num);
	p_download->file_size = toy_vfile_getsize(p_download->md5_array, p_download->blocks_num);
	u_int *exist_bytes_field = (u_int*)(strchr(p_download->filemsg, 0) + 1);
	u_int exist_bytes = *exist_bytes_field;
	p_download->have_down_len = exist_bytes;
	if(exist_bytes != 0)
	{
		u_char *md5_field = (u_char*)exist_bytes_field + 4;
		u_int for_num = (exist_bytes - 1) / BLOCK_SIZE + 1;
		u_int i;
		u_int exist_blocks = 0;
		for(i = 0; i < for_num; ++i)
		{
			if(memcmp(md5_field + 16 * i, p_download->md5_array + 16 * i, 16) == 0)
				exist_blocks = i;
			else
				break;
		}
		if(p_download->have_down_len > exist_blocks * BLOCK_SIZE)
			p_download->have_down_len = exist_blocks * BLOCK_SIZE;
	}

	if(toy_download_if_done(p_download))
	{
		toy_download_done(p_download);
		return ;
	}

	u_int msg_len = 8 + p_download->blocks_num * 16;
	send(p_download->fd, &msg_len, sizeof(u_int), 0);
	send(p_download->fd, &p_download->have_down_len, sizeof(u_int), 0);
	send(p_download->fd, &p_download->file_size, sizeof(u_int), 0);
	send(p_download->fd, p_download->md5_array, p_download->blocks_num * 16, 0);

	toy_selcycle_fdset_add_fd(p_download->fd, FT_WRITE);
	p_download->state = DS_TRANSFERING;

	p_download->data_buf = malloc(BUF_SIZE);
	p_download->buf_size = BUF_SIZE;
	toy_download_read_from_block(p_download);
}

void toy_download_recvmsg(struct toy_download_s *p_download)
{
	int recv_ret;
	int fd = p_download->fd;
	if(p_download->done_len == -1)
	{
		recv_ret = recv(fd, &p_download->msg_len, sizeof(u_int), 0);
		if(recv_ret == 0)
		{
			toy_download_abort(p_download);
			return ;
		}
		if(recv_ret == -1)
		{
			toy_warning("recv() error");
			return ;
		}
		p_download->filemsg = malloc(p_download->msg_len);
		p_download->done_len = 0;
	}

	recv_ret = recv(fd, p_download->filemsg + p_download->done_len, p_download->msg_len - p_download->done_len, 0);
	if(recv_ret == 0)
	{
		toy_upload_abort(p_download);
		return ;
	}
	if(recv_ret == -1)
	{
		toy_warning("recv() error");
		return ;
	}
	p_download->done_len += recv_ret;
	if(p_download->done_len == p_download->msg_len)
		toy_download_recv_filemsg_finished(p_download);
}

void toy_download_transfer(struct toy_download_s *p_download)
{
	u_int send_len;
	send_len = send(p_download->fd, p_download->data_buf + p_download->send_off, p_download->buf_size - p_download->send_off, 0);
	p_download->send_off += send_len;
	p_download->have_down_len += send_len;

	if(toy_download_if_done(p_download))
	{
		toy_download_done(p_download);
		return ;
	}

	if(p_download->send_off == p_download->buf_size)
		toy_download_read_from_block(p_download);
}

void toy_download_handler_on_recv(struct toy_download_s *p_download)
{
	if(p_download->state == DS_LISTEN)
		toy_download_accept(p_download);
	else if(p_download->state == DS_CONNECTED)
		toy_download_recvmsg(p_download);
	else
		toy_download_abort(p_download);
}

void toy_download_handler_on_send(struct toy_download_s *p_download)
{
	if(p_download->state == DS_TRANSFERING)
		toy_download_transfer(p_download);
}