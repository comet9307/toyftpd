#ifndef _TOY_USER_H_
#define _TOY_USER_H_

#include <sys/types.h>

struct toy_node_s
{
	char *user_name;
	u_int user_id;
	struct toy_node_s *left;
	struct toy_node_s *right;
	int balance;
};

struct toy_index_s 
{
	struct toy_node_s *root;
};

struct toy_user_s
{
	int users_num;
	struct toy_index_s users_index;
};

extern struct toy_user_s toy_user_global;

int toy_user_get_id_by_name(const char *user_name, u_int *id_buf);
int toy_user_check_pwd(u_int user_id, const u_char *pwdhash);


#endif // _TOY_USER_H_