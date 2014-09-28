#include "tool.h"
#include "block.h"
#include "conf.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>


int toy_block_open(const u_char *md5)
{
	char blockname[33];
	toy_tool_md5_to_str(md5, blockname);
	u_int dir_len = strlen(toy_conf_global.option_data_storage_dir);
	char *pathname = malloc(dir_len + 33);
	strcpy(pathname, toy_conf_global.option_data_storage_dir);
	strcpy(pathname + dir_len, blockname);
	int fd = open(pathname, O_RDWR);
	if(fd == -1 && errno == ENOENT)
		return -1;
	if(fd == -1)
		toy_warning("open file error() on toy_block_open");
	return fd;
}

int toy_block_new(const u_char *md5, u_int total_bytes)
{
	char blockname[33];
	toy_tool_md5_to_str(md5, blockname);
	u_int dir_len = strlen(toy_conf_global.option_data_storage_dir);
	char *pathname = malloc(dir_len + 33);
	strcpy(pathname, toy_conf_global.option_data_storage_dir);
	strcpy(pathname + dir_len, blockname);
	int fd = open(pathname, O_RDWR | O_CREAT);
	u_int exist_bytes = 0;
	write(fd, &exist_bytes, 4);
	write(fd, &total_bytes, 4);
	return fd;
}

u_int toy_block_get_progress(int fd)
{
	u_int exist_bytes;
	u_int offset = lseek(fd, 0, SEEK_CUR);
	lseek(fd, 0, SEEK_SET);
	read(fd, &exist_bytes, 4, 0);
	lseek(fd, offset, SEEK_SET);
	return exist_bytes;
}

void toy_block_updateinfo(struct toy_blockinfo_s *p_blockinfo)
{
	u_int offset = lseek(p_blockinfo->fd, 0, SEEK_CUR);
	lseek(p_blockinfo->fd, 0, SEEK_SET);
	write(p_blockinfo->fd, &p_blockinfo->exist_bytes, 4);
	lseek(p_blockinfo->fd, offset, SEEK_SET);
}

void toy_block_close(int fd)
{
	close(fd);
}

void toy_block_update(int fd, const u_char *buffer, u_int write_len)
{
	lseek(fd, 0, SEEK_SET);
	u_int exist_bytes;
	read(fd, &exist_bytes, 4);

	lseek(fd, 8 + exist_bytes, SEEK_SET);
	write(fd, buffer, write_len);
	exist_bytes += write_len;

	lseek(fd, 0, SEEK_SET);
	write(fd, &exist_bytes, 4);
}

int toy_block_read(const u_char *md5, u_char *data_buf, u_int offset, u_int buf_size)
{
	int fd = toy_block_open(md5);
	lseek(fd, offset, SEEK_SET);
	int read_len = read(fd, data_buf, buf_size);
	toy_block_close(fd);
	return read_len;
}