#ifndef _TOY_CLIENT_H_
#define _TOY_CLIENT_H_

#include <sys/types.h>

struct toy_client_s
{
	int fd;
	
	enum CLIENT_STATE
	{
		CS_UNLOGIN,
		CS_WAITPWD,
		CS_LOGINED
	} state;
	u_int user_id;
	struct toy_client_s *next;
};

struct toy_client_pool_s
{
	u_int clients_num;
	struct toy_client_s *first_link;
};

extern struct toy_client_pool_s toy_client_global_pool;

void toy_client_add(int client_fd);
void toy_client_handler(struct toy_client_s *p_client);

#endif // _TOY_CLIENT_H_