#include "client.h"
#include "upload.h"
#include "tool.h"
#include "select_cycle.h"
#include "conf.h"
#include "download.h"

#include <sys/socket.h>
#include <netinet/in.h>

struct toy_select_cycle_s 
{
	fd_set rd_set;
	fd_set wr_set;
	fd_set excep_set;
	int max_fd;

} toy_select_cycle_global;


void toy_selcycle_fdset_add_fd(int fd, enum FDSET_TYPE set_type)
{
	if(set_type == FT_READ)
		FD_SET(fd, &toy_select_cycle_global.rd_set);
	else if(set_type == FT_WRITE)
		FD_SET(fd, &toy_select_cycle_global.wr_set);
	else
		FD_SET(fd, &toy_select_cycle_global.excep_set);

	if(fd > toy_select_cycle_global.max_fd)
		toy_select_cycle_global.max_fd = fd;

	// must judge if the number greater than FD_SETSIZE
}


void toy_selcycle_fdset_rm_fd(int fd, enum FDSET_TYPE set_type)
{
	if(set_type == FT_READ)
		FD_CLR(fd, &toy_select_cycle_global.rd_set);
	else if(set_type == FT_WRITE)
		FD_CLR(fd, &toy_select_cycle_global.wr_set);
	else 
		FD_CLR(fd, &toy_select_cycle_global.excep_set);
}

int toy_selcycle_isset(int fd, enum FDSET_TYPE set_type)
{
	if(set_type == FT_READ)
		return FD_ISSET(fd, &toy_select_cycle_global.rd_set);
	else if(set_type == FT_WRITE)
		return FD_ISSET(fd, &toy_select_cycle_global.wr_set);
	return FD_ISSET(fd, &toy_select_cycle_global.excep_set);
}

int toy_accept_client(int monitor_fd)
{
	struct sockaddr_in cli_addr;
	socklen_t addrlen = sizeof(cli_addr);
	int client_fd = accept(monitor_fd, (struct sockaddr*)&cli_addr, &addrlen);
	if(client_fd == -1)
		toy_warning("accept error");
/*	if(rd_set_num == FD_SETSIZE - 1)
	{
		close(client_fd);
		toy_warning("rd_set is full, accept failed");
	}*/
	return client_fd;
}

void toy_select_cycle(int monitor_fd)
{
	FD_ZERO(&toy_select_cycle_global.rd_set);
	FD_ZERO(&toy_select_cycle_global.wr_set);
	toy_selcycle_fdset_add_fd(monitor_fd, FT_READ);

	fd_set rdset_tmp;
	fd_set wrset_tmp;
	int fd_ready;
	for(;;)
	{
		rdset_tmp = toy_select_cycle_global.rd_set;
		wrset_tmp = toy_select_cycle_global.wr_set;
		fd_ready = select(toy_select_cycle_global.max_fd + 1, &rdset_tmp, &wrset_tmp, 0, 0);
		// never happened
		if(fd_ready == 0)
		{

		}
		else if( fd_ready == -1 )
		{
			// do something on select error
			toy_warning("select error");
		}
		else
		{
			if(FD_ISSET(monitor_fd, &rdset_tmp))
			{  
				int client_fd = toy_accept_client(monitor_fd);
				toy_client_add(client_fd);
				toy_selcycle_fdset_add_fd(client_fd, FT_READ);
			} // if FD_ISSET(listen_fd, &rdset_tmp)

			// clients
			struct toy_client_s *p_client = toy_client_global_pool.first_link;
			while(p_client)
			{
				struct toy_client_s *p_tmp = p_client->next;
				if(FD_ISSET(p_client->fd, &rdset_tmp))
					toy_client_handler(p_client);
				p_client = p_tmp;
			}

			struct toy_upload_s *p_upload = toy_upload_global_pool.first_link;
			while(p_upload)
			{
				struct toy_upload_s *p_tmp = p_upload->next;
				if(FD_ISSET(p_upload->fd, &rdset_tmp))
					toy_upload_handler(p_upload);
				p_upload = p_tmp;
			}

			struct toy_download_s *p_download = toy_download_global_pool.first_link;
			while(p_download)
			{
				struct toy_download_s *p_tmp = p_download->next;
				if(FD_ISSET(p_download->fd, &rdset_tmp))
					toy_download_handler_on_recv(p_download);
				if(FD_ISSET(p_download->fd, &wrset_tmp))
					toy_download_handler_on_send(p_download);
				p_download = p_tmp;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////

int toy_create_clients_monitor_socket()
{
	int listen_fd;
	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(listen_fd == -1)
		toy_kill("create listen socket failed");

	struct sockaddr_in serv_addr;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(21);   // FTP 监听端口

	// one server may have more than one ip address, bind one 
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	if(bind(listen_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
		toy_kill("bind listen socket failed");

	if(listen(listen_fd, toy_conf_global.option_max_clients) == -1)
		toy_kill("listen failed");
	return listen_fd;
}
