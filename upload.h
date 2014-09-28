#ifndef _TOY_UPLOAD_H_
#define _TOY_UPLOAD_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

struct toy_upload_s
{
	enum UPLOAD_STATE
	{
		US_LISTEN,
		US_CONNECTED,
		US_UPLOADING
	} state;
	int fd;
	u_int user_id;

	u_char *filemsg;
	u_int msg_len;
	int done_len;

	u_char *data_buf;
	u_int buf_size;
	u_int data_len;

	u_int blocks_num;
	struct toy_blockinfo_s *blockinfo_array;
	struct toy_upload_s *next;
};
struct toy_upload_pool_s
{
	u_int uploads_num;
	struct toy_upload_s *first_link;
};
extern struct toy_upload_pool_s toy_upload_global_pool;

int toy_upload_add(u_int user_id, u_short *port_buf);
void toy_upload_delete(struct toy_upload_s *p_upload);
void toy_upload_handler(struct toy_upload_s *p_upload);

#endif // _TOY_UPLOAD_H_