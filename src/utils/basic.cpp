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

#include "basic.h"

int system_call(const char *__restrict cmd, ...) {
    char buffer[CMD_LEN];
    va_list args;
    va_start(args, cmd);
    vsprintf(buffer, cmd, args);
    va_end(args);
    return system(buffer);
}

char *stradd(const char *__restrict a, const char *__restrict b) {
    char *buffer = new char[strlen(a) + strlen(b) + 1];
    sprintf(buffer, "%s%s", a, b);
    return buffer;
}

char *stradd(const char *__restrict a, const char *__restrict b, const char *__restrict c) {
    char *buffer = new char[strlen(a) + strlen(b) + strlen(c) + 1];
    sprintf(buffer, "%s%s%s", a, b, c);
    return buffer;
}

char *strbase(const char *__restrict str, char chr) {
    const char *pos = strrchr(str, chr);
    if (pos == NULL) return strdup("");
    else return strndup(str, pos - str);
}

char *strext(const char *__restrict str, char chr) {
    const char *pos = strrchr(str, chr);
    if (pos == NULL) return strdup(str);
    else return strdup(pos + 1);
}

int strcount(const char *__restrict str, char chr) {
    int count = 0;
    for (const char *pos = str; *pos != 0; pos++) {
        if (*pos == chr) count ++;
    }
    return count;
}

char *strtrim(const char *__restrict str) {
    const char *begin = str;
    const char *end = str + strlen(str);
    while (begin < end) {
        if (isspace(*begin)) begin ++;
        else if (isspace(*(end-1))) end --;
        else break;
    }
    if (begin == end) return strdup("");
    else return strndup(begin, end - begin);
}

int strfit(const char *__restrict str, const char *__restrict pattern) {
    int len = strlen(pattern);
    int ret = -1;
    if (pattern[0] == '*' and pattern[len - 1] == '*') {
        char *substr = strndup(pattern + 1, len - 2);
        if (strstr(str, substr) >= 0) ret = 0;
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

char *strreplace(const char *__restrict str, const char *__restrict from, const char *__restrict to) {
    if (strstr(str, from) == NULL) return strdup(str);

    int lenstr = strlen(str);
    int lenfrom = strlen(from);
    int lento = strlen(to);
    int count = 0;
    for (const char *p = str; p != NULL; p += lenfrom) {
        p = strstr(p, from);
        if (p == NULL) break;
        count ++;
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

// 将str中前length个字符转化为16进制
char* str2hex(const char *str, long length){
    // Must initial to 0 here!
    char *buffer = new char[length * 2 + 1]();
    for (int i = 0; i < length; i++) {
        sprintf(buffer + i * 2, "%02X", str[i]);
    }
    return buffer;
}
