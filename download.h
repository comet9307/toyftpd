#ifndef _TOY_DOWNLOAD_H_
#define _TOY_DOWNLOAD_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

struct toy_download_s
{
	enum DOWNLOAD_STATE
	{
		DS_LISTEN,
		DS_CONNECTED,
		DS_TRANSFERING
	} state;
	int fd;
	u_int user_id;

	u_char *filemsg;
	u_int msg_len;
	u_int done_len;
	
	u_char *md5_array;
	u_int blocks_num;
	u_int file_size;
	u_int have_down_len;

	u_char *data_buf;
	u_int buf_size;
	u_int send_off;
	
	struct toy_download_s *next;
};

struct toy_download_pool_s 
{
	u_int downloads_num;
	struct toy_download_s *first_link;
};

extern struct toy_download_pool_s toy_download_global_pool;

int toy_download_add(u_int user_id, u_short *port_buf);

void toy_download_handler_on_recv(struct toy_download_s *p_download);
void toy_download_handler_on_send(struct toy_download_s *p_download);

#endif // _TOY_DOWNLOAD_H_