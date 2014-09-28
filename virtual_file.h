#ifndef _TOY_VFILE_H_
#define _TOY_VFILE_H_

#include <sys/types.h>

void toy_vfile_add(u_int user_id, const char *path_name, u_int blocks_num , const u_char *md5_field);
//int toy_vfile_check_md5(u_int user_id, const char *path_name, u_int for_num, const u_char *md5_field);
void toy_vfile_get_md5(u_int user_id, const char *path_name, u_char **md5_array_buf, u_int *blocks_num_buf);
int toy_vfile_getsize(const u_char *md5_array, u_int blocks_num);
void toy_vfile_create(const u_int user_id);

#endif // _TOY_VFILE_H_