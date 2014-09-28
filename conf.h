#ifndef _TOY_CONF_H_
#define _TOY_CONF_H_

#include <sys/types.h>

struct toy_conf_s
{
    u_int option_max_clients;
    char *option_userinfos_file;
    char *option_pwdhashs_file;
    char *option_users_dir;
    char *option_data_storage_dir;
};
extern struct toy_conf_s toy_conf_global;

void toy_conf_init();
void toy_conf_load_config(char *conf_path);
void toy_conf_check();

#endif // _TOY_CONF_H_