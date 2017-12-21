/*
 * monitor.cpp
 *
 *  Created on: Jan 10, 2017
 *      Author: sunchao
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/inotify.h>

#include "monitor.h"
#include "log.h"
#include "basic.h"
#include "file.h"

Monitor::Monitor() {
    _monitor = inotify_init();
    if (_monitor < 0) {
        log.error("[MONITOR] CANNOT start monitor!");
    }
    _watcher = -1;
    _path = NULL;
    _pattern = NULL;
    _notity = NULL;
    _params = NULL;
}

Monitor::~Monitor() {
    stop();
    if (_monitor >= 0) {
        close(_monitor);
        _monitor = -1;
    }
}

void Monitor::set(const char *path, const char *pattern, notify_t notify,
        void *params) {
    if (_monitor < 0)
        return;
    stop();
    if (is_exist(path) == 0) {
        log.info("[MONITOR] No dir %s, make it!", path);
        create_dir(path);
    }
    uint32_t watch_mask = IN_MOVED_TO | IN_CLOSE_WRITE;
    _watcher = inotify_add_watch(_monitor, path, watch_mask);
    if (_watcher < 0) {
        log.error("[MONITOR] CANNOT start watcher!");
    } else {
        _path = strdup(path);
        _pattern = strdup(pattern);
        _notity = notify;
        _params = params;
        log.info("[MONITOR] Start monitor on %s for pattern(%s).", _path,
                _pattern);
    }
}

void Monitor::walk(notify_t notify, void *params) {
    DIR *dir = opendir(_path);
    if (dir == NULL)
        return;
    dirent *ptr = readdir(dir);
    while (ptr != NULL) {
        if (ptr->d_type == 8) {
            if (ptr->d_name[0] == '.') {
                log.info("[MONITOR] Automatic ignore hidden file %s/%s.", _path,
                        ptr->d_name);
            } else if (strfit(ptr->d_name, _pattern) == 0) {
                (*notify)(ptr->d_name, params);
            }
        }
        ptr = readdir(dir);
    }
    closedir(dir);
}

void Monitor::check() {
//    log.debug("[MONITOR] begin checking ...");
    if (_watcher < 0)
        return;
    int buffer_size = 0;
    if (ioctl(_monitor, FIONREAD, &buffer_size) < 0) {
        log.debug("[MONITOR] FIONREAD!");
        return;
    }
    log.debug("[MONITOR] Buffer size: %d", buffer_size);
    int esize = sizeof(inotify_event);
    if (buffer_size < esize)
        return;
    char *buffer = new char[buffer_size];
    int actual_read = 0;
    while (actual_read < buffer_size) {
        actual_read += read(_monitor, buffer + actual_read,
                buffer_size - actual_read);
    }
    for (int pos = 0; pos < actual_read;) {
        inotify_event *event = (inotify_event *) (buffer + pos);
        pos += sizeof(inotify_event) + event->len;
        if (event->len <= 0)
            continue;

        if ((event->mask & (IN_MOVED_TO | IN_CLOSE_WRITE)) == 0) {
            log.error("[MONITOR] Unknown Monitor Mask: %s/%s %d.", _path,
                    event->name, event->mask);
        } else {
            if (event->name[0] == '.') {
                log.info("[MONITOR] Automatic ignore hidden file %s/%s.", _path,
                        event->name);
            } else if (strfit(event->name, _pattern) == 0) {
                log.info("[MONITOR] File Changed: %s/%s.", _path, event->name);
                (*_notity)(event->name, _params);
            } else {
                log.debug("[MONITOR] Unhandled file changed: %s/%s.", _path,
                        event->name);
            }
        }
    }
    delete buffer;
}

void Monitor::stop() {
    if (_monitor < 0)
        return;
    if (_watcher >= 0) {
        log.info("[MONITOR] Stop monitor on %s of pattern(%s).", _path,
                _pattern);
        inotify_rm_watch(_monitor, _watcher);
        _watcher = -1;
    }
    if (_pattern != NULL) {
        delete _pattern;
        _path = NULL;
        _pattern = NULL;
    }
    if (_path != NULL) {
        delete _path;
        _path = NULL;
    }
    if (_notity != NULL) {
        (*_notity)(NULL, _params);      // 清理_params
        _params = NULL;
        _notity = NULL;
    }
}
