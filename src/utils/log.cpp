/*
 * log.cpp
 *
 *  Created on: Oct 23, 2016
 *      Author: sunchao
 */

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>

#include "log.h"
#include "basic.h"

#define EXTLEN  32

Log::Log() {
    _dir = NULL;
    _logfile = NULL;
    _logfp = NULL;
    _level = 15;
    _this_day = 0;
}

Log::~Log() {
    info("[LOG] Close the log by PID: %d.", getpid());
    if (_dir)
        delete _dir;
    if (_logfile)
        delete _logfile;
    if (_logfp) {
        delete _logfp;
        _logfp = NULL;
    }
}

void Log::init(const char *dir) {
    _dir = strdup(dir);
    _logfile = new char[strlen(dir) + EXTLEN];
    _logfile[0] = 0;
    create_dir(_dir);
    open_new();
}

bool Log::started() {
    return _logfile != NULL and _logfile[0] != 0;
}

void Log::setLogLevel(int level) {
    _level = level;
}

void Log::debug(const char *msg, ...) {
    if ((_level & DEBUG_LEVEL) == 0)
        return;

    va_list args;
    va_start(args, msg);
    write(0, "DEBUG", msg, args);
    va_end(args);
}

void Log::info(const char *msg, ...) {
    if ((_level & INFO_LEVEL) == 0)
        return;

    va_list args;
    va_start(args, msg);
    write(0, "INFO ", msg, args);
    va_end(args);
}

void Log::error(const char *msg, ...) {
    if ((_level & ERROR_LEVEL) == 0)
        return;

    va_list args;
    va_start(args, msg);
    write(0, "ERROR", msg, args);
    va_end(args);
}

/* 独立于 LEVEL 体系，不加前置时间戳和 LEVEL，同 printf() */
void Log::pure(const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    write(1, "ERROR", msg, args);
    va_end(args);
}

/* 独立于 LEVEL 体系，不加前置时间戳和 LEVEL，同 pure()，但会在开头空格，使内容对齐 */
void Log::sublog(const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    write(2, "ERROR", msg, args);
    va_end(args);
}

/*
 * 计算出上一天的日志名
 * 若今天20171210，则得到 _dir/20171209.log
 * 若文件存在，压缩之
 */
void Log::close_old() {
    if (_logfp) {
        delete _logfp;
        _logfp = NULL;
    }

    time_t now_time;
    struct tm yesterday;
    char short_fn[16];

    time(&now_time);
    now_time -= 24 * 3600;
    localtime_r(&now_time, &yesterday);
    sprintf(short_fn, "%04d%02d%02d.log", yesterday.tm_year + 1900,
            yesterday.tm_mon + 1, yesterday.tm_mday);
    char *path = stradd(_dir, "/", short_fn);
    if (is_exist(path)) {
        gzip_compress(path);
    }
    delete path;
}

/*
 * 更新 _logfile, _this_day
 * 压缩上一天的日志
 * 初始化 及 到了第二天 时调用
 */
void Log::open_new() {
    close_old();

    time_t now_time;
    struct tm now;

    time(&now_time);
    localtime_r(&now_time, &now);
    sprintf(_logfile, "%s/%04d%02d%02d.log", _dir, now.tm_year + 1900,
            now.tm_mon + 1, now.tm_mday);
    _this_day = now.tm_mday;

    _logfp = new File(_logfile, "a+");
    if (_logfp == NULL) {
        printf("[LOG] [ERROR] Cannot open file %s\n", _logfile);
    }
    info("[LOG] Open the log by PID: %d.", getpid());
}

/*
 * @parm mode 若为 0，表示添加前置时间戳和 LEVEL
 *            若为 1，表示不加前置时间戳和 LEVEL，同 printf()，参数 level 失效
 *            若为 2，表示不加前置时间戳和 LEVEL，同 printf()，但会在开头空格，使内容对齐，参数 level 失效
 */
void Log::write(int mode, const char *level, const char *msg, va_list args) {
    if (!started())
        return;

    time_t now_time;
    struct tm now;
    char buffer[EXTLEN];

    time(&now_time);
    localtime_r(&now_time, &now);
    strftime(buffer, 24, "%F %T", &now);

//    // NOTE: args不能使用两次
//    if ((_level & CONSOLE_LEVEL) != 0) {
//        printf("%s [%s] ", buffer, level);
//        vprintf(msg, args);
//        printf("\n");
//    }
//
//    if (!_logfp->is_null()) {
//        _logfp->print("%s [%s] ", buffer, level);
//        _logfp->vprint(msg, args);
//        _logfp->print("\n");
//    } else {
//        printf("[ERROR] [LOG] Empty _logfp!\n");
//    }

    char time_buf[50];              // 此条日志的开头，"2018-01-16 15:59:17 [INFO ] "
    char log_buf[500];              // 此条日志的实际内容
    sprintf(time_buf, "%s [%s] ", buffer, level);
    vsprintf(log_buf, msg, args);
    char* print_buf;                // 此条日志的最终内容
    if (mode == 0)
        print_buf = stradd(time_buf, log_buf, "\n");
    else if (mode == 1)
        print_buf = log_buf;
    else
        print_buf = stradd("                            ", log_buf);

    if ((_level & CONSOLE_LEVEL) != 0 || mode > 0) {
        printf("%s", print_buf);
    }

    if (_logfp == NULL || _logfp->is_null()) {
        printf("[ERROR] [LOG] Empty _logfp!\n");
        if (mode != 1) delete print_buf;
        return;
    } else {
        _logfp->print("%s", print_buf);
    }

    if (now.tm_mday != _this_day) {
        _logfp->print("[LOG] Another day!\n");
        open_new();
    }
    if (mode != 1) delete print_buf;
}

Log log;
