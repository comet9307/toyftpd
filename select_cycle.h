#ifndef _TOY_SELCYCLE_H_
#define _TOY_SELCYCLE_H_

enum FDSET_TYPE
{
	FT_READ,
	FT_WRITE,
	FT_EXCPETION
};


void toy_selcycle_fdset_add_fd(int fd, enum FDSET_TYPE set_type);
void toy_selcycle_fdset_rm_fd(int fd, enum FDSET_TYPE set_type);
void toy_select_cycle(int monitor_fd);
int toy_selcycle_isset(int fd, enum FDSET_TYPE set_type);

int toy_create_clients_monitor_socket();

#endif // _TOY_SELCYCLE_H_