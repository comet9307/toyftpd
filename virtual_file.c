#include "virtual_file.h"
#include "tool.h"
#include "conf.h"
#include "protocal.h"
#include "block.h"

#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/fcntl.h>	

static int toy_vfile_open_index(u_int user_id, int flags)
{
	char id_str[6];
	toy_tool_uint_to_str5(user_id, id_str);

	char *index_name = malloc(strlen(toy_conf_global.option_users_dir) + strlen(id_str) + sizeof("index"));
	strcpy(index_name, toy_conf_global.option_users_dir);
	strcat(index_name, id_str);
	strcat(index_name, "index");

	int index_fd = open(index_name, flags);
	free(index_name);
	return index_fd;
}

static int toy_vfile_open_map(u_int user_id, int flags)
{
	char id_str[6];
	toy_tool_uint_to_str5(user_id, id_str);

	char *map_name = malloc(strlen(toy_conf_global.option_users_dir) + strlen(id_str) + sizeof("map"));
	strcpy(map_name, toy_conf_global.option_users_dir);
	strcat(map_name, id_str);
	strcat(map_name, "map");

	int map_fd = open(map_name, flags);
	free(map_name);
	return map_fd;
}

void toy_vfile_add(u_int user_id, const char *path_name, u_int blocks_num , const u_char *md5_field)
{
	int map_fd = toy_vfile_open_map(user_id, O_RDWR);
	struct stat stat_buf;
	fstat(map_fd, &stat_buf);
	u_int offset = stat_buf.st_size;
	if(blocks_num != 0)
	{
		lseek(map_fd, 0, SEEK_END);
		write(map_fd, &blocks_num, sizeof(u_int));
		write(map_fd, md5_field, blocks_num * 16);
	}
	close(map_fd);

	int index_fd = toy_vfile_open_index(user_id, O_RDWR);
	lseek(index_fd, 0, SEEK_END);
	u_int size = strlen(path_name) + 1;
	write(index_fd, &size, sizeof(u_int));
	write(index_fd, path_name, size);
	if(blocks_num == 0)
		offset = -1;
	write(index_fd, &offset, sizeof(u_int));
	close(index_fd);
}

/*
int toy_vfile_check_md5(u_int user_id, const char *path_name, u_int for_num, const u_char *md5_field)
{
	int index_fd = toy_vfile_open_index(user_id, O_RDONLY);
	struct stat stat_buf;
	fstat(index_fd, &stat_buf);
	u_int file_size = stat_buf.st_size;
	u_char *file_buf = malloc(file_size);
	read(index_fd, file_buf, file_size);
	close(index_fd);

	u_char *cur_item = file_buf;
	char *cur_path;
	int map_offset = -1;
	while(cur_item < file_buf + file_size)
	{
		cur_path = cur_item + sizeof(u_int);
		if(strcmp(cur_path, path_name) == 0)
		{
			map_offset = *(int*)(strchr(cur_path, 0) + 1);
			break;
		}
		cur_item = strchr(cur_path, 0) + sizeof(u_int) + 1;
	}	
	free(file_buf);

	if(map_offset == -1)
		return -1;

	int map_fd = toy_vfile_open_map(user_id, O_RDONLY);
	lseek(map_fd, map_offset, SEEK_SET);
	u_char *md5 = malloc(for_num * 16);
	read(map_fd, md5, for_num * 16);
	int if_check_pass;
	if(memcpy(md5_field, md5, for_num * 16) == 0)
		if_check_pass = 1;
	else
		if_check_pass = 0;
	free(md5);
	close(map_fd);
	return if_check_pass;
}*/

void toy_vfile_get_md5(u_int user_id, const char *path_name, u_char **md5_array_buf, u_int *blocks_num_buf)
{
	int index_fd = toy_vfile_open_index(user_id, O_RDONLY);
	struct stat stat_buf;
	fstat(index_fd, &stat_buf);
	u_int file_size = stat_buf.st_size;
	u_char *file_buf = malloc(file_size);
	read(index_fd, file_buf, file_size);
	close(index_fd);

	u_char *cur_item = file_buf;
	char *cur_path;
	int map_offset = -1;
	while(cur_item < file_buf + file_size)
	{
		cur_path = cur_item + sizeof(u_int);
		if(strcmp(cur_path, path_name) == 0)
		{
			map_offset = *(int*)(strchr(cur_path, 0) + 1);
			break;
		}
		cur_item = strchr(cur_path, 0) + sizeof(u_int) + 1;
	}	
	free(file_buf);

	if(map_offset == -1)
		return ;

	int map_fd = toy_vfile_open_map(user_id, O_RDONLY);
	lseek(map_fd, map_offset, SEEK_SET);
	u_int blocks_num;
	read(map_fd, &blocks_num, sizeof(u_int));
	u_char *md5_array = malloc(blocks_num * 16);
	read(map_fd, md5_array, blocks_num * 16);

	*blocks_num_buf = blocks_num;
	*md5_array_buf = md5_array;

	close(map_fd);
}

int toy_vfile_getsize(const u_char *md5_array, u_int blocks_num)
{
	int fd = toy_block_open(md5_array + (blocks_num - 1) * 16);
	struct stat stat_buf;
	fstat(fd, &stat_buf);
	u_int file_size = (blocks_num - 1) * BLOCK_SIZE + stat_buf.st_size - sizeof(struct toy_block_header);
	toy_block_close(fd);
	return file_size;
}

int toy_vfile_list(u_int user_id, const char *dir, char *list_buf, int buf_len)
{
	int index_fd = toy_vfile_open_index(user_id, O_RDONLY);	
	struct stat stat_buf;
	fstat(index_fd, &stat_buf);
	u_int file_size = stat_buf.st_size;
	u_char *file_buf = malloc(file_size);
	read(index_fd, file_buf, file_size);
	close(index_fd);

	u_char *cur_item = file_buf;
	u_int name_len;
	u_int req_len = 0;
	char *cur_path;
	u_int dir_len = strlen(dir);
	while(cur_item < file_buf + file_size)
	{
		cur_path = cur_item + sizeof(u_int);
		if(strncmp(cur_path, dir, dir_len) == 0)
		{
			name_len = strlen(cur_path);
			if(req_len + name_len + 1 <= buf_len)
			{
				strcpy(list_buf + req_len, cur_path);
				req_len = req_len + name_len + 1;
				list_buf[req_len - 1] = '\n';
			}
			else
				req_len = req_len + name_len + 1;
		}
		cur_item = strchr(cur_path, 0) + sizeof(u_int) + 1;
	}	

	free(file_buf);
	return req_len;
}

void toy_vfile_create(u_int user_id)
{
	int index_fd = toy_vfile_open_index(user_id, O_CREAT | O_RDWR);
	int map_fd = toy_vfile_open_map(user_id, O_CREAT);
	close(map_fd);

	u_int size[4] = {
		sizeof("/document/"),
		sizeof("/music/"),
		sizeof("/picture/"),
		sizeof("/video/")
	};
	char *str[4] = {
		"/document/",
		"/music/",
		"/picture/",
		"/video/"
	};

	u_int offset = -1;
	int i;
	for(i = 0; i < 4; ++i)
	{
		write(index_fd, size+i, sizeof(u_int));
		write(index_fd, str[i], size[i]);
		write(index_fd, &offset, sizeof(offset));
	}

	close(index_fd);
}