#ifndef _TOY_TOOL_H_
#define _TOY_TOOL_H_

#include <sys/types.h>

void toy_warning(const char *msg);
void toy_kill(const char *msg);
void toy_tool_md5_to_str(const u_char *md5, char *str_buf);
int toy_is_uint(const char *str);
int toy_max(int num1, int num2);
void toy_tool_uint_to_str5(u_int num, char *str_buf);

#endif // _TOY_TOOL_H_