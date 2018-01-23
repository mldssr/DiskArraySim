/*
 * basic.cpp
 *
 *  Created on: Jan 12, 2017
 *      Author: sunchao
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>

#include "basic.h"

/* 调用系统命令，返回值0为成功。 */
int system_call(const char *__restrict cmd, ...) {
    char buffer[CMD_LEN];
    va_list args;
    va_start(args, cmd);
    vsprintf(buffer, cmd, args);
    va_end(args);
    return system(buffer);
}

/*
 * 连接字符串，返回新的完整的字符串。
 * @return a newly allocated string
 */
char *stradd(const char *__restrict a, const char *__restrict b) {
    char *buffer = new char[strlen(a) + strlen(b) + 1];
    sprintf(buffer, "%s%s", a, b);
    return buffer;
}

/*
 * 连接字符串，返回新的完整的字符串。
 * @return a newly allocated string
 */
char *stradd(const char *__restrict a, const char *__restrict b,
        const char *__restrict c) {
    char *buffer = new char[strlen(a) + strlen(b) + strlen(c) + 1];
    sprintf(buffer, "%s%s%s", a, b, c);
    return buffer;
}

/*
 * 以特定字符右分割字符串的前半部分。
 * @return a newly allocated string
 */
char *strbase(const char *__restrict str, char chr) {
    const char *pos = strrchr(str, chr);
    if (pos == NULL)
        return strdup("");
    else
        return strndup(str, pos - str);
}

/*
 * 以特定字符右分割字符串的后半部分。
 * @return a newly allocated string
 */
char *strext(const char *__restrict str, char chr) {
    const char *pos = strrchr(str, chr);
    if (pos == NULL)
        return strdup(str);
    else
        return strdup(pos + 1);
}

/* 返回字符串中特定字符出现的次数。 */
int strcount(const char *__restrict str, char chr) {
    int count = 0;
    for (const char *pos = str; *pos != 0; pos++) {
        if (*pos == chr)
            count++;
    }
    return count;
}

/*
 * 返回去掉前后空白的字符串：
 * - ' ' space
 * - '\f' formfeed
 * - '\n' newline
 * - '\r' carriage return
 * - '\t' horizontal tab
 * - '\v' vertical tab
 * @return a newly allocated string
 */
char *strtrim(const char *__restrict str) {
    const char *begin = str;
    const char *end = str + strlen(str);
    while (begin < end) {
        if (isspace(*begin))
            begin++;
        else if (isspace(*(end - 1)))
            end--;
        else
            break;
    }
    if (begin == end)
        return strdup("");
    else
        return strndup(begin, end - begin);
}

/* 检测字符串是否符合给定模式（只允许前后有*）。 */
int strfit(const char *__restrict str, const char *__restrict pattern) {
    int len = strlen(pattern);
    int ret = -1;
    if (pattern[0] == '*' and pattern[len - 1] == '*') {
        char *substr = strndup(pattern + 1, len - 2);
        if (strstr(str, substr) >= 0)
            ret = 0;
        delete substr;
    } else if (pattern[0] == '*') {
        int pos = strlen(str) - len + 1;
        ret = strcmp(str + pos, pattern + 1);
    } else if (pattern[len - 1] == '*') {
        ret = strncmp(str, pattern, len - 1);
    } else {
        ret = strcmp(str, pattern);
    }
    return ret;
}

/*
 * 替换str中的from为to。
 * @return a newly allocated string
 */
char *strreplace(const char *__restrict str, const char *__restrict from,
        const char *__restrict to) {
    if (strstr(str, from) == NULL)
        return strdup(str);

    int lenstr = strlen(str);
    int lenfrom = strlen(from);
    int lento = strlen(to);
    int count = 0;
    for (const char *p = str; p != NULL; p += lenfrom) {
        p = strstr(p, from);
        if (p == NULL)
            break;
        count++;
    }
    char *buffer = new char[lenstr + (lento - lenfrom) * count + 1];
    buffer[0] = 0;
    for (const char *p = str; p != NULL; p += lenfrom) {
        const char *nxt = strstr(p, from);
        if (nxt == NULL) {
            strcat(buffer, p);
            break;
        } else {
            strncat(buffer, p, nxt - p);
            strcat(buffer, to);
            p = nxt;
        }
    }
    return buffer;
}

/*
 * 将str中前length个字符转化为16进制
 * @return a newly allocated string
 */
char *str2hex(const char *str, long length) {
    // Must initial to 0 here!
    char *buffer = new char[length * 2 + 1]();
    for (int i = 0; i < length; i++) {
        sprintf(buffer + i * 2, "%02X", str[i]);
    }
    buffer[length * 2] = 0;
    return buffer;
}

/*
 * 将time_t转化为地方时间str，例如 "2017-12-12 19:03:45"
 * @parm buf 存储结果，需要调用者提供空间，至少20byte
 */
void time_t2str(time_t time, char *buf, size_t buf_size) {
    struct tm ptm;
    localtime_r(&time, &ptm);
    strftime(buf, buf_size, "%F %T", &ptm);
}

/*
 * 将地方时间str，例如 "2017-12-12 19:03:45, 转化为time_t"
 */
time_t str2time_t(const char *time_str) {
    struct tm ptm;
    strptime(time_str, "%Y-%m-%d %H:%M:%S", &ptm);
    // 按当地时区解析tmp_time
    time_t time = mktime(&ptm);
    return time;
}

static bool sranded = 0;
/*
 * 返回[min, max]的随机整数
 */
int get_random(int min, int max) {
    if (sranded == 0) {
        srand((unsigned) time(NULL));
        sranded = 1;
    }
    int ret = (rand() % (max - min + 1)) + min;
    return ret;
}

double get_random(double min, double max) {
    if (sranded == 0) {
        srand((unsigned) time(NULL));
        sranded = 1;
    }
    double f = (double) rand() / RAND_MAX;
    return min + f * (max - min);
}
