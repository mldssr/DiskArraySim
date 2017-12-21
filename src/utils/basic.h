/*
 * basic.h
 *
 *  Created on: Jan 12, 2017
 *      Author: sunchao
 */

#ifndef BASIC_H_
#define BASIC_H_

#define CMD_LEN  512

/* 调用系统命令，返回值0为成功。 */
int system_call(const char *__restrict cmd, ...);

/* 连接字符串，返回新的完整的字符串。 */
char *stradd(const char *__restrict a, const char *__restrict b);
char *stradd(const char *__restrict a, const char *__restrict b, const char *__restrict c);

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
char *strreplace(const char *__restrict str, const char *__restrict from, const char *__restrict to);

/* 将str中前length个字符转化为16进制 */
char* str2hex(const char *str, long length);

#endif /* BASIC_H_ */
