/*
 * log.h
 *
 *  Created on: Oct 22, 2016
 *      Author: sunchao
 */

#ifndef LOG_H_
#define LOG_H_

#include "file.h"

/* 日志级别：测试信息。 */
const int DEBUG_LEVEL = 1;
/* 日志级别：一般提示信息。 */
const int INFO_LEVEL = 2;
/* 日志级别：错误信息。 */
const int ERROR_LEVEL = 4;
/* 日志级别：同时打印到控制台。 */
const int CONSOLE_LEVEL = 8;

class Log {
private:
    char *_dir;             // 日志文件保存的文件夹
    char *_logfile;         // 日志文件名，_dir/20171209.log
    File *_logfp;           // 日志文件描述符
    int _this_day;          // 日志文件的当前日期
    int _level;             // 日志的保存级别

    /* 打开新的日志文件。如果日志文件名存在，则将其解压缩并打开。 */
    void open_new();
    /* 关闭日志文件。如果当前已打开日志文件，则将其关闭并压缩。 */
    void close_old();
    /* 记录日志信息。参数：日志级别，信息，信息的参数。 */
    void write(int mode, const char *level, const char *msg, va_list args);

public:
    Log();
    ~Log();

    /* 初始化日志模块。设置日志文件的存储目录。 */
    void init(const char *dir);

    /* 判断日志模块是否正常开启。用于主程序。 */
    /* used for main program to test whether started */
    bool started();

    /* 设置日志模块的记录级别。 */
    void setLogLevel(int level);

    /* 记录测试信息。 */
    void debug(const char *msg, ...);
    /* 记录一般提示信息。 */
    void info(const char *msg, ...);
    /* 记录错误信息。 */
    void error(const char *msg, ...);
    /* 独立于 LEVEL 体系，不加时间戳和 LEVEL，同 printf() */
    void pure(const char *msg, ...);
    /* 独立于 LEVEL 体系，不加时间戳和 LEVEL，同 pure()，但会在开头空格，使内容对齐 */
    void sublog(const char *msg, ...);
};

/* 全局变量。便于记录日志。 */
extern Log log;

#endif /* LOG_H_ */
