#include "conf.h"
#include "select_cycle.h"
#include "tool.h"
#include "defs.h"


int main(int argc, char *argv[])
{
	toy_conf_init();

	toy_conf_load_config(DEF_CONF_PATH);

	toy_conf_check();

	toy_user_init_infos();


	// go into listen connection 
	int monitor_fd = toy_create_clients_monitor_socket();
	toy_select_cycle(monitor_fd);
	return 0;
}