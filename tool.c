#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void toy_print_msg(const char *msg)
{
	printf("%s\n", msg);
}

void toy_warning(const char *msg)
{
	toy_print_msg(msg);
}

void toy_kill(const char *msg)
{
	printf("%s\n", msg);
	exit(1);
}

void toy_tool_md5_to_str(const u_char *md5, char *str_buf)
{
	u_int i;
	u_char tmp;
	char buf[3];
	for(i = 0; i < 16; ++i)
	{
		tmp = (md5[i] << 4) + (md5[i] >> 4);
		sprintf(buf, "%02x", tmp);
		memcpy(str_buf + i * 2, buf, 2);
	}
	str_buf[32] = 0;
}

void toy_tool_uint_to_str5(u_int num, char *str_buf)
{
	str_buf[5] = 0;
	int i;
	for(i = 4; i >= 0; --i)
	{
		str_buf[i] = num % 10 + '0';
		num /= 10;
	}
}

int toy_is_uint(const char *str)
{
	return 1;
}

int toy_max(int num1, int num2)
{
	if(num1 - num2 > 0)
		return num1;
	return num2;
}