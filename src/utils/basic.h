/*
 * basic.h
 *
 *  Created on: Jan 12, 2017
 *      Author: sunchao
 */
#include "time.h"

#ifndef BASIC_H_
#define BASIC_H_

#define CMD_LEN  512

#define typeof(x) __typeof__(x)
#define MAX(a,b) ({typeof(a) A=a,B=b; A > B ? A : B;})

/* 调用系统命令，返回值0为成功。 */
int system_call(const char *__restrict cmd, ...);

/* 调用系统命令，并获取输出结果（仅第一行，不包含\n），输出缓冲区由用户提供，失败时返回NULL。 */
char *system_call(char *output, int size, const char *__restrict cmd, ...);

/* 连接字符串，返回新的完整的字符串。 */
char *stradd(const char *__restrict a, const char *__restrict b);
char *stradd(const char *__restrict a, const char *__restrict b,
        const char *__restrict c);

/* 以特定字符右分割字符串的前半部分。 */
char *strbase(const char *__restrict str, char chr);

/* 以特定字符右分割字符串的后半部分。 */
char *strext(const char *__restrict str, char chr);

/* 返回字符串中特定字符出现的次数。 */
int strcount(const char *__restrict str, char chr);

/* 返回去掉前后空白的字符串。 */
char *strtrim(const char *__restrict str);

/* 检测字符串是否符合给定模式（只允许前后有*）。 */
int strfit(const char *__restrict str, const char *__restrict pattern);

/* 替换str中的from为to。 */
char *strreplace(const char *__restrict str, const char *__restrict from,
        const char *__restrict to);

/* 将str中前length个字符转化为16进制 */
char *str2hex(const char *str, long length);

/*
 * 将time_t转化为地方时间str，例如 "2017-12-12 19:03:45"
 * @parm buf 存储结果，需要调用者提供空间，至少20byte
 */
void time_t2str(time_t time, char *buf, size_t buf_size);

/*
 * 将地方时间str，例如 "2017-12-12 19:03:45, 转化为time_t"
 */
time_t str2time_t(const char *time);

/*
 * 返回[min, max]的随机整数
 */
int get_random(int min, int max);

double get_random(double min, double max);


#endif /* BASIC_H_ */
