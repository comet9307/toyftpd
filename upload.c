#include "select_cycle.h"
#include "block.h"
#include "virtual_file.h"
#include "tool.h"	
#include "protocal.h"
#include "upload.h"

#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sys/socket.h>

#define BUF_SIZE					8192


struct toy_upload_pool_s toy_upload_global_pool;

void toy_upload_update_blockinfos(struct toy_upload_s *p_upload);
void toy_upload_abort(struct toy_upload_s *p_upload);

int toy_upload_add(u_int user_id, u_short *port_buf)
{
	 struct toy_upload_s *ptr = malloc(sizeof(struct toy_upload_s));
	 ptr->next = toy_upload_global_pool.first_link;
	 toy_upload_global_pool.first_link = ptr;

	 ptr->state = US_LISTEN;
	 ptr->fd = -1;
	 ptr->user_id = user_id;

	 ptr->filemsg = NULL;
	 ptr->msg_len = 0;
	 ptr->done_len = 0;

	 ptr->data_buf = NULL;
	 ptr->buf_size = 0;
	 ptr->data_len = 0;

	 ptr->blocks_num = 0;
	 ptr->blockinfo_array = NULL;
	 ++toy_upload_global_pool.uploads_num;


	int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(listen_fd == -1)
	{
		toy_upload_abort(ptr);
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

void toy_upload_delete(struct toy_upload_s *p_upload)
{
	toy_upload_update_blockinfos(p_upload);

	if(p_upload == toy_upload_global_pool.first_link)
		toy_upload_global_pool.first_link = p_upload->next;
	else
	{
		struct toy_upload_s *ptr = toy_upload_global_pool.first_link;
		while(ptr->next != p_upload)
			ptr = ptr->next;
		ptr->next = p_upload->next;
	}

	if(p_upload->fd != -1)
	{
		if(toy_selcycle_isset(p_upload->fd, FT_READ))
			toy_selcycle_fdset_rm_fd(p_upload->fd, FT_READ);
		if(toy_selcycle_isset(p_upload->fd, FT_WRITE))
			toy_selcycle_fdset_rm_fd(p_upload->fd, FT_WRITE);
		close(p_upload->fd);
	}
	if(p_upload->filemsg)
		free(p_upload->filemsg);
	if(p_upload->blockinfo_array)
		free(p_upload->blockinfo_array);
	if(p_upload->data_buf)
		free(p_upload->data_buf);
	free(p_upload);
	
	--toy_upload_global_pool.uploads_num;
}

void toy_upload_done(struct toy_upload_s *p_upload)
{
	toy_upload_delete(p_upload);
}

void toy_upload_abort(struct toy_upload_s *p_upload)
{
	toy_upload_delete(p_upload);
}

void toy_upload_accept(struct toy_upload_s *p_upload)
{
	struct sockaddr_in remote_addr;
	socklen_t addr_len;
	int fd = accept(p_upload->fd, (struct sockaddr*)&remote_addr, &addr_len);
	toy_selcycle_fdset_rm_fd(p_upload->fd, FT_READ);
	close(p_upload->fd);
	if(fd == -1)
	{
		toy_upload_abort(p_upload);
		return ;
	}
	toy_selcycle_fdset_add_fd(fd, FT_READ);
	p_upload->fd = fd;
	p_upload->state = US_CONNECTED;
}

void toy_upload_update_blockinfos(struct toy_upload_s *p_upload)
{
	u_int i;
	for(i = 0; i < p_upload->blocks_num; ++i)
	{
		toy_block_updateinfo(p_upload->blockinfo_array + i);
		toy_block_close(p_upload->blockinfo_array[i].fd);
	}
}

void toy_upload_recv_filemsg_finished(struct toy_upload_s *p_upload)
{	
	char *filepath = p_upload->filemsg;
	u_int *blknum_field = (u_int*)(strchr(p_upload->filemsg, 0) + 1);
	u_int blocks_num = *blknum_field;
	u_int *file_size_field = blknum_field + 1;
	u_int file_size = *file_size_field;
	u_char *md5_field = (u_char*)file_size_field + 4;
	toy_vfile_add(p_upload->user_id, filepath, blocks_num, md5_field);
	p_upload->blocks_num = blocks_num;

	p_upload->blockinfo_array = malloc(blocks_num * sizeof(struct toy_blockinfo_s));
	u_int i;
	u_int if_done = 1;
	struct toy_blockinfo_s *p_blockinfo = p_upload->blockinfo_array;
	for(i = 0; i < blocks_num; ++i)
	{
		if(i == blocks_num - 1)
			p_blockinfo->total_bytes = file_size - (blocks_num-1) * BLOCK_SIZE;
		else
			p_blockinfo->total_bytes = BLOCK_SIZE;
		memcpy(p_blockinfo->md5, md5_field + 16*i, 16);
		p_blockinfo->fd = toy_block_open(p_blockinfo->md5);
		if(p_blockinfo->fd == -1)
			p_blockinfo->fd = toy_block_new(p_blockinfo->md5, p_blockinfo->total_bytes);
		p_blockinfo->exist_bytes = toy_block_get_progress(p_blockinfo->fd);

		send(p_upload->fd, &p_blockinfo->exist_bytes, sizeof(u_int), 0);
		if(p_blockinfo->total_bytes - p_blockinfo->exist_bytes > 0)
			if_done = 0;
		++p_blockinfo;

	}
	free(p_upload->filemsg);
	p_upload->filemsg = NULL;
	p_upload->msg_len = 0;
	p_upload->done_len = -1;
	if(if_done == 1)
	{
		toy_upload_done(p_upload);
		return ;
	}
	p_upload->state = US_UPLOADING;

	p_upload->data_buf = malloc(BUF_SIZE);
	p_upload->buf_size = BUF_SIZE;
}

void toy_upload_recv_filemsg(struct toy_upload_s *p_upload)
{
	int recv_ret;
	int fd = p_upload->fd;
	if(p_upload->done_len == 0)
	{
		recv_ret = recv(fd, &p_upload->msg_len, sizeof(u_int), 0);
		if(recv_ret == 0)
		{
			toy_upload_abort(p_upload);
			return ;
		}
		if(recv_ret == -1)
		{
			toy_warning("recv() error");
			return ;
		}
		p_upload->filemsg = malloc(p_upload->msg_len);
	}

	recv_ret = recv(fd, p_upload->filemsg + p_upload->done_len, p_upload->msg_len - p_upload->done_len, 0);
	if(recv_ret == 0)
	{
		toy_upload_abort(p_upload);
		return ;
	}
	if(recv_ret == -1)
	{
		toy_warning("recv() error");
		return ;
	}
	p_upload->done_len += recv_ret;
	if(p_upload->done_len == p_upload->msg_len)
		toy_upload_recv_filemsg_finished(p_upload);
}

void toy_upload_write_to_block(struct toy_upload_s *p_upload)
{
	struct toy_blockinfo_s *p_blockinfo = p_upload->blockinfo_array;
	u_int block_ord;
	for(block_ord = 0; block_ord < p_upload->blocks_num; ++block_ord)
	{
		if(p_blockinfo->total_bytes - p_blockinfo->exist_bytes > 0)
			break;
		++p_blockinfo;
	}
	if(block_ord >= p_upload->blocks_num)
		return ;

	u_char *buffer = p_upload->data_buf;
	u_int data_len = p_upload->data_len;
	u_int write_len;
	u_char *p_data;

	p_data = buffer;
	while(p_upload->data_len > 0)
	{
		if(p_upload->data_len >= p_blockinfo->total_bytes - p_blockinfo->exist_bytes)
		{
			write_len = p_blockinfo->total_bytes - p_blockinfo->exist_bytes;
			toy_block_update(p_blockinfo->fd, p_data, write_len);
			p_blockinfo->exist_bytes += write_len;
			while(p_blockinfo->total_bytes - p_blockinfo->exist_bytes == 0)
			{
				++p_blockinfo;
				++block_ord;
				if(block_ord >= p_upload->blocks_num)
					return ;
			}
		}
		else
		{
			write_len = p_upload->data_len;
			toy_block_update(p_blockinfo->fd, p_data, write_len);
			p_blockinfo->exist_bytes += write_len;
		}

		p_upload->data_len -= write_len;
		p_data = p_data + write_len;
	}
}

int toy_upload_if_done(struct toy_upload_s *p_upload)
{
	u_int i;
	struct toy_blockinfo_s *p_blockinfo = p_upload->blockinfo_array;
	for(i = 0; i < p_upload->blocks_num; ++i)
	{
		if(p_blockinfo->total_bytes - p_blockinfo->exist_bytes > 0)
			return 0;
		++p_blockinfo;
	}
	return 1;
}


void toy_upload_transfer(struct toy_upload_s *p_upload)
{
	u_int recv_len;
	recv_len = recv(p_upload->fd, p_upload->data_buf + p_upload->data_len, p_upload->buf_size - p_upload->data_len, 0);
	p_upload->data_len += recv_len;

	if(p_upload->data_len == p_upload->buf_size)
		toy_upload_write_to_block(p_upload);
	if(recv_len == 0)
	{
		if(p_upload->data_len != 0)
			toy_upload_write_to_block(p_upload);
		if(toy_upload_if_done(p_upload))
			toy_upload_done(p_upload);
		else
			toy_upload_abort(p_upload);
	}
}


void toy_upload_handler(struct toy_upload_s *p_upload)
{
	if(p_upload->state == US_LISTEN)
		toy_upload_accept(p_upload);
	else if(p_upload->state == US_CONNECTED)
	{
		toy_upload_recv_filemsg(p_upload);
//		int flags = fcntl(p_upload->fd, F_GETFL, 0);
//		fcntl(p_upload->fd, F_SETFL, flags | O_NONBLOCK);
	}
	else
		toy_upload_transfer(p_upload);
}