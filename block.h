#ifndef _TOY_BLOCK_H_
#define _TOY_BLOCK_H_

#include <sys/types.h>

struct toy_block_header
{
	u_int total_bytes;
	u_int exist_bytes;
};

struct toy_blockinfo_s
{
	u_char md5[16];
	u_int total_bytes;
	u_int exist_bytes;
	int fd;
};

int toy_block_open(const u_char *md5);
int toy_block_new(const u_char *md5, u_int total_bytes);
u_int toy_block_get_progress(int fd);
void toy_block_updateinfo(struct toy_blockinfo_s *p_blockinfo);
void toy_block_close(int fd);
void toy_block_update(int fd, const u_char *buffer, u_int write_len);
int toy_block_read(const u_char *md5, u_char *data_buf, u_int offset, u_int buf_size);

#endif // _TOY_BLOCK_H_