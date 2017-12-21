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
    write("DEBUG", msg, args);
    va_end(args);
}

void Log::info(const char *msg, ...) {
    if ((_level & INFO_LEVEL) == 0)
        return;

    va_list args;
    va_start(args, msg);
    write("INFO ", msg, args);
    va_end(args);
}

void Log::error(const char *msg, ...) {
    if ((_level & ERROR_LEVEL) == 0)
        return;

    va_list args;
    va_start(args, msg);
    write("ERROR", msg, args);
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

void Log::write(const char *level, const char *msg, va_list args) {
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

    char time_buf[50];
    char log_buf[500];
    sprintf(time_buf, "%s [%s] ", buffer, level);
    vsprintf(log_buf, msg, args);
    char* print_buf = stradd(time_buf, log_buf, "\n");

    if ((_level & CONSOLE_LEVEL) != 0) {
        printf("%s", print_buf);
    }
    if (!_logfp->is_null()) {
        _logfp->print("%s", print_buf);
    } else {
        printf("[ERROR] [LOG] Empty _logfp!\n");
    }

    if (now.tm_mday != _this_day) {
        printf("[LOG] Another day!\n");
        open_new();
    }
    delete print_buf;
}

Log log;
