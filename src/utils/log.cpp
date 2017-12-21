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
    _level = 7;
    _this_day = 0;
}

Log::~Log() {
    info("[LOG] Close the log by PID: %d.", getpid());
    if (_dir) delete _dir;
    if (_logfile) delete _logfile;
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

void Log::close_old() {
    time_t now_time;
    struct tm yesterday;
    char short_fn[16];

    time(&now_time);
    now_time -= 24 * 3600;
    localtime_r(&now_time, &yesterday);
    sprintf(short_fn, "%04d%02d%02d.log",
            yesterday.tm_year + 1900, yesterday.tm_mon + 1, yesterday.tm_mday);
    char *path = stradd(_dir, "/", short_fn);
    if (is_exist(path)) {
        gzip_compress(path);
    }
    delete path;
}

void Log::open_new() {
    time_t now_time;
    struct tm now;

    time(&now_time);
    localtime_r(&now_time, &now);
    sprintf(_logfile, "%s/%04d%02d%02d.log",
            _dir, now.tm_year + 1900, now.tm_mon + 1, now.tm_mday);

     _this_day = now.tm_mday;
    info("[LOG] Open the log by PID: %d.", getpid());

    close_old();
}

void Log::write(const char *level, const char *msg, va_list args) {
    if (!started()) return;

    time_t now_time;
    struct tm now;
    char buffer[EXTLEN];

    time(&now_time);
    localtime_r(&now_time, &now);
    strftime(buffer, 24, "%F %T", &now);

    File *logfp = new File(_logfile, "a+");
    if (!logfp->is_null()) {
        logfp->print("%s [%s] ", buffer, level);
        logfp->vprint(msg, args);
        logfp->print("\n");
        logfp->close();
    }
    delete logfp;
    if (now.tm_mday != _this_day) {
        open_new();
    }
}

Log log;
