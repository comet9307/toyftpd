#include "tool.h"
#include "conf.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

struct toy_conf_s toy_conf_global;

struct toy_uint_opt_s
{
    char *option_name;
    u_int *p_option_value;
};
static struct toy_uint_opt_s uint_opt_array[] = {
    {"max_clients", &toy_conf_global.option_max_clients}, 
    {0, 0}
};

struct toy_str_opt_s
{
    char *option_name;
    char **p_option_value;
};
static struct toy_str_opt_s str_opt_arrary[] = {
    {"userinfos_file", &toy_conf_global.option_userinfos_file}, 
    {"pwdhashs_file", &toy_conf_global.option_pwdhashs_file},
    {"users_dir", &toy_conf_global.option_users_dir},
    {"data_storage_dir", &toy_conf_global.option_data_storage_dir},
    {0, 0}
};

void toy_conf_init()
{
    toy_conf_global.option_max_clients = 1024;
    toy_conf_global.option_userinfos_file = "userinfos";
    toy_conf_global.option_pwdhashs_file = "pwdhashs";
    toy_conf_global.option_users_dir = "users_dir/";
    toy_conf_global.option_data_storage_dir = "data/";
}

void toy_conf_load_file(char *confpath, char **p_conf_buf)
{
    struct stat conf_fd_stat;
    if(stat(confpath, &conf_fd_stat) < 0)
        toy_kill("get config file stat failed");

    u_int file_len = conf_fd_stat.st_size;
    if(file_len == 0)
        return ;
    if(file_len > 102400)
        toy_kill("config file size should be less than 100 KB");

    int conf_fd = open(confpath, O_RDWR);
    if(conf_fd == -1)
        toy_kill("open config file failed");

    *p_conf_buf = malloc(file_len + 1);
    int rd_len = read(conf_fd, *p_conf_buf, file_len);
    *p_conf_buf[rd_len] = 0;
    if(rd_len == -1)
        toy_kill("open config file failed");
}

int toy_conf_set_option(char *opt_name, char *opt_value)
{
    // uint options
    struct toy_uint_opt_s *p_uint_opt = uint_opt_array;
    while( p_uint_opt->option_name != 0)
    {
        if( strcmp( p_uint_opt->option_name, opt_name) == 0 )
        {
            if(toy_is_uint(opt_value) == 0)
                return 0;
            *p_uint_opt->p_option_value = atoi(opt_value);
            return 1;
        }
        ++p_uint_opt;
    }

    //string options
    struct toy_str_opt_s *p_str_opt = str_opt_arrary;
    while( p_str_opt->option_name != 0)
    {
        if(strcmp( p_str_opt->option_name, opt_name) == 0)
        {
            u_int value_len = sizeof(opt_value);
            if( *p_str_opt->p_option_value != NULL)
                free(*p_str_opt->p_option_value);
            *p_str_opt->p_option_value = malloc(value_len + 1);
            strcpy(*p_str_opt->p_option_value, opt_value);
            return 1;
        }
        ++p_str_opt;
    }
    return 0;
}

void toy_conf_parse_line(char *line_ptr, u_int line_len, u_int line_ord)
{
    u_int char_pos = 0;
    // skip space
    while(line_ptr[char_pos] == ' ' || line_ptr[char_pos] == '\t')
        ++char_pos;
    if(line_ptr[char_pos] == '\n')
        return ;
    if(line_ptr[char_pos] == '#')
        return ;

    // get option name
    char *opt_name_ptr = line_ptr + char_pos;
    u_int opt_name_len = 0;
    while(line_ptr[char_pos] != ' ' && line_ptr[char_pos] != '\t' && line_ptr[char_pos] != '=')
    {
        ++char_pos;
        ++opt_name_len;
    }

    // seek '='
    while(line_ptr[char_pos] != ' ' && line_ptr[char_pos] != '\t')
        ++char_pos;
    if(line_ptr[char_pos] != '=')
        goto conf_warning;
    while(line_ptr[char_pos] != ' ' && line_ptr[char_pos] != '\t')
        ++char_pos;
    if(line_ptr[char_pos] == '\n')
        goto conf_warning;

    // get option value
    char *opt_value_ptr = line_ptr + char_pos;
    u_int opt_value_len = 0;
    while(line_ptr[char_pos] != ' ' && line_ptr[char_pos] != '\t' && line_ptr[char_pos] != '\n')
    {
        ++char_pos;
        ++opt_value_len;
    }

    // check any other character
    while(line_ptr[char_pos] != '\n')
    {
        if(line_ptr[char_pos] != ' ' && line_ptr[char_pos] != '\t')
            goto conf_warning;
        ++char_pos;
    }

    // set option 
    char *opt_name = malloc(opt_name_len + 1);
    memcpy(opt_name, opt_name_ptr, opt_name_len);
    opt_name[opt_name_len] = 0;
    char *opt_value = malloc(opt_value_len + 1);
    memcpy(opt_value, opt_value_ptr, opt_value_len);
    opt_value[opt_value_len] = 0;
    int retval = toy_conf_set_option(opt_name, opt_value);
    free(opt_value);
    free(opt_name);
    if(retval == 0)
        goto conf_warning;
    return ;
conf_warning:
{
    char msg[100];
    sprintf(msg, "warning : illegal usage on config file line %d", line_ord);
    toy_warning(msg);
}
}

void toy_conf_fill_config(char *conf_buf)
{
    char *cur_char_ptr = conf_buf;
    char *cur_line_ptr = cur_char_ptr; 
    u_int cur_line_ord = 0;
    u_int line_len;
    while(*cur_char_ptr != 0)
    {
        ++cur_line_ord;
        cur_line_ptr = cur_char_ptr;
        line_len = 0;
        while(*cur_char_ptr != '\n' && *cur_char_ptr != 0)
        {
            ++line_len;
            ++cur_char_ptr;
        }
        toy_conf_parse_line(cur_line_ptr, line_len, cur_line_ord);
        if(*cur_char_ptr == '\n')
            ++cur_char_ptr;
    }
}

void toy_conf_load_config(char *conf_path)
{
    char *conf_buf = NULL;
    toy_conf_load_file(conf_path, &conf_buf);
    if(conf_buf == NULL)
        return;
    toy_conf_fill_config(conf_buf);
    if(conf_buf)
        free(conf_buf);
}

void toy_conf_check()
{
    struct stat stat_buf;
    int retval = stat(toy_conf_global.option_data_storage_dir, &stat_buf);
    if(retval == -1)
    {
        if(errno == ENOENT)
            toy_kill("data_storage_dir is not exist!");
        else
            toy_kill("error on toy_conf_check() ");
    }
    if(!S_ISDIR(stat_buf.st_mode))
        toy_kill("config option data_storage_dir is not directory!");

    retval = stat(toy_conf_global.option_users_dir, &stat_buf);
    if(retval == -1)
    {
        if(errno == ENOENT)
            toy_kill("user_dir is not exist!");
        else
            toy_kill("error on toy_conf_check() ");
    }
    if(!S_ISDIR(stat_buf.st_mode))
        toy_kill("config option users_dir is not directory!");

    retval = stat(toy_conf_global.option_userinfos_file, &stat_buf);
    if(retval == -1)
    {
        if(errno == ENOENT)
            toy_kill("userinfos_file is not exist!");
        else
            toy_kill("error on toy_conf_check() ");
    }

    retval = stat(toy_conf_global.option_pwdhashs_file, &stat_buf);
    if(retval == -1)
    {
        if(errno == ENOENT)
            toy_kill("pwdhashs_file is not exist!");
        else
            toy_kill("error on toy_conf_check() ");
    }
}