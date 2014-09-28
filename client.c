#include "user.h"
#include "upload.h"
#include "protocal.h"
#include "client.h"
#include "download.h"

#include <stdlib.h>
#include <string.h>

struct toy_client_pool_s toy_client_global_pool;

void toy_client_add(int client_fd)
{
	struct toy_client_s *ptr = malloc(sizeof(struct toy_client_s));
	ptr->next = toy_client_global_pool.first_link;
	toy_client_global_pool.first_link = ptr;

	toy_client_global_pool.first_link->fd = client_fd;
	toy_client_global_pool.first_link->state = CS_UNLOGIN;
	toy_client_global_pool.first_link->user_id = -1;
	++toy_client_global_pool.clients_num;
}

void toy_client_break(struct toy_client_s *p_client)
{
	if(p_client == toy_client_global_pool.first_link)
		toy_client_global_pool.first_link = p_client->next;
	else
	{
		struct toy_client_s *p_pre = toy_client_global_pool.first_link;
		while(p_pre->next != p_client)
			p_pre = p_pre->next;
		p_pre->next = p_client->next;
	}
	free(p_client);	
	--toy_client_global_pool.clients_num;
}

enum PACK_TYPE toy_client_parse_pack_type(const u_char *pack_buf)
{
	u_char u_type = *pack_buf;
	return (enum PACK_TYPE) u_type;
}

void toy_client_register(struct toy_client_s *p_client, u_char *pack_buf)
{
	u_char send_buf[1];
	u_int buf_len = sizeof(send_buf);

	const char *user = pack_buf + 1;
	const char *pwdhash = strchr(pack_buf + 1, 0) + 1;
	int ret = toy_user_register(user, pwdhash);
	if(ret == 0)
		send_buf[0] = (u_char)PT_HAVE_EXISTED;
	else
		send_buf[0] = (u_char)PT_REGISTER_SUCCEED;
	send(p_client->fd, &buf_len, sizeof(buf_len), 0);
	send(p_client->fd, send_buf, buf_len, 0);
}

void toy_client_do_user(struct toy_client_s *p_client, u_char *pack_buf)
{
	u_char send_buf[1];
	u_int buf_len = sizeof(send_buf);

	if(p_client->state != CS_UNLOGIN)
		send_buf[0] = (u_char)PT_HAVE_LOGINED;
	else if(toy_user_get_id_by_name(pack_buf + 1, &p_client->user_id) == 0)
		send_buf[0] = (u_char)PT_USER_NOEXIST;
	else
	{
		send_buf[0] = (u_char)PT_REQU_PWDHASH;
		p_client->state = CS_WAITPWD;
	}
	send(p_client->fd, &buf_len, sizeof(buf_len), 0);
	send(p_client->fd, send_buf, sizeof(send_buf), 0);
}

void toy_client_do_pwdhash(struct toy_client_s *p_client, u_char *pack_buf)
{
	u_char send_buf[1];
	u_int buf_len = sizeof(send_buf);
	if( toy_user_check_pwd(p_client->user_id, pack_buf + 1) == 0)
	{
		send_buf[0] = (u_char)PT_WRONG_PWD;
		p_client->state = CS_UNLOGIN;
	}
	else
	{
		send_buf[0] = (u_char)PT_LOGIN_SUCCEED;
		p_client->state = CS_LOGINED;
	}
	send(p_client->fd, &buf_len, sizeof(buf_len), 0);
	send(p_client->fd, send_buf, sizeof(send_buf), 0);
}

void toy_client_do_logout(struct toy_client_s *p_client, u_char *pack_buf)
{
	u_char send_buf[1];
	u_int buf_len = sizeof(send_buf);
	p_client->state = CS_UNLOGIN;
	send(p_client->fd, &buf_len, sizeof(buf_len), 0);
	send(p_client->fd, send_buf, sizeof(send_buf), 0);
	p_client->user_id = -1;
}

void toy_client_do_upload(struct toy_client_s *p_client, u_char *pack_buf)
{
	if(p_client->state != CS_LOGINED)
		return;
	u_short port;
	if(toy_upload_add(p_client->user_id, &port) == 0)
	{
		return ;
	}
	u_char send_buf[7];
	send_buf[0] = (u_char)PT_UPLOAD_ESTABLISH;
	*(u_int*)(send_buf + 1) = *(u_int*)(pack_buf + 1);
	*(u_short*)(send_buf + 5) = htons(port);
	u_int buf_len = sizeof(send_buf);
	send(p_client->fd, &buf_len, sizeof(buf_len), 0);
	send(p_client->fd, send_buf, buf_len, 0);
}

void toy_client_do_download(struct toy_client_s *p_client, u_char *pack_buf)
{
	if(p_client->state != CS_LOGINED)
		return ;
	u_short port;
	if(toy_download_add(p_client->user_id, &port) == 0)
		return ;
	u_char send_buf[7];
	send_buf[0] = (u_char)PT_DOWNLOAD_ESTABLISH;
	*(u_int*)(send_buf + 1) = *(u_int*)(pack_buf + 1);
	*(u_short*)(send_buf + 5) = htons(port);
	u_int buf_len = sizeof(send_buf);
	send(p_client->fd, &buf_len, sizeof(buf_len), 0);
	send(p_client->fd, send_buf, buf_len, 0);
}

void toy_client_do_list(struct toy_client_s *p_client, u_char *pack_buf)
{
	char *dir = pack_buf + 1;
	char *list_buf = malloc(1024);
	int req_len = toy_vfile_list(p_client->user_id, dir, list_buf, 1024);
	if(req_len > 1024)
	{
		free(list_buf);
		list_buf = malloc(req_len);
		toy_vfile_list(p_client->user_id, dir, list_buf, req_len);
	}
	send(p_client->fd, &req_len, sizeof(req_len), 0);
	send(p_client->fd, list_buf, req_len, 0);
	free(list_buf);
}

void toy_client_do_unknown()
{
	toy_warning("toy_client_do_unknown...");
}

void toy_client_handler(struct toy_client_s *p_client)
{
	int fd = p_client->fd;
	enum CLIENT_STATE state = p_client->state;
	u_int pack_len;
	int recv_ret = recv(fd, &pack_len , sizeof(u_int), 0);
	if(recv_ret == 0)
	{
		toy_client_break(p_client);
		return ;
	}
	if(recv_ret == -1)
	{
		toy_warning("recv() error");
		return;
	}
	u_char *pack_buf = malloc(pack_len);
	// judge recv return -1 ?
	recv(fd, pack_buf, pack_len, 0);

	enum PACK_TYPE pk_type = toy_client_parse_pack_type(pack_buf);
	if(pk_type == PT_REGISTER)
		toy_client_register(p_client, pack_buf);
	else if(pk_type == PT_USER)
		toy_client_do_user(p_client, pack_buf);
	else if(pk_type == PT_PWDHASH)
		toy_client_do_pwdhash(p_client, pack_buf);
	else if(pk_type == PT_LOGOUT)
		toy_client_do_logout(p_client, pack_buf);
	else if(pk_type == PT_UPLOAD)
		toy_client_do_upload(p_client, pack_buf);
	else if(pk_type == PT_DOWNLOAD)
		toy_client_do_download(p_client, pack_buf);
	else if(pk_type == PT_LIST)
		toy_client_do_list(p_client, pack_buf);
	else // pk_type == PT_UNKNOWN
		toy_client_do_unknown();

	free(pack_buf);
}