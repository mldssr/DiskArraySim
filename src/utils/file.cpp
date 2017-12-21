/*
 * file.cpp
 *
 *  Created on: Dec 20, 2016
 *      Author: sunchao
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "file.h"
#include "basic.h"
#include "log.h"

File::File(const char *filename, const char *modes) {
    if (strlen(filename) > 0) {
        while (1) {
            _fp = fopen(filename, modes);
            if (_fp == NULL) {
                if (!is_exist(filename)) {
                    log.error("[FILE] Can not open file %s", filename);
                    break;
                }
            } else {
                break;
            }
            usleep(999999);
        }
    } else {
        _fp = NULL;
    }
}

File::File(const char *dir, const char *filename, const char *modes) {
    char *full_path = NULL;
    if (dir == NULL) {
        full_path = strdup(filename);
    } else {
        full_path = stradd(dir, "/", filename);
    }
    while (1) {
        _fp = fopen(full_path, modes);
        if (_fp == NULL) {
            if (!is_exist(filename)) {
                log.error("[FILE] Can not open file %s", filename);
                break;
            }
        } else {
            break;
        }
        usleep(999999);
    }
    delete full_path;
}

File::~File() {
    close();
}

bool File::is_null() {
    return _fp == NULL;
}

bool File::is_eof() {
    if (_fp == NULL)
        return true;
    else
        return feof(_fp) != 0;
}

size_t File::size() {
    if (is_null())
        return 0;
    long int current_pos, size;
    current_pos = ftell(_fp);
    fseek(_fp, 0, SEEK_END);
    size = ftell(_fp);
    fseek(_fp, current_pos, SEEK_SET);
    return size;
}

void File::seek(long pos) {
    if (is_null())
        return;
    fseek(_fp, pos, SEEK_SET);
}

void File::read(void *data, size_t size, size_t count) {
    if (is_eof())
        return;
    size_t actual_read = fread(data, size, count, _fp);
    if (actual_read < count) {
        fprintf(stderr, "Read file ERROR!\n");
        exit(101);
    }
}

/*
 * 读取一行字符
 * NOTE: 读取一行，最多读取 size-1 个字符，最后一位为 \0
 * FIXME 读完最后一行，依然每到EOF，fgets()返回NULL
 */
void File::readline(char *data, size_t size) {
    if (is_eof())
        return;
    data = fgets(data, size, _fp);
}

int File::scan(const char *msg, ...) {
    if (is_eof())
        return -1;
    va_list args;
    va_start(args, msg);
    int ret = vfscanf(_fp, msg, args);
    va_end(args);
    if (ret >= 0)
        return ret;
    return ret;
}

void File::write(const void *data, size_t size, size_t count) {
    if (is_null())
        return;
    size_t actual_write = fwrite(data, size, count, _fp);
    if (actual_write < count) {
        fprintf(stderr, "Write file ERROR!\n");
        exit(102);
    }
}

void File::print(const char *msg, ...) {
    if (is_null())
        return;
    va_list args;
    va_start(args, msg);
    vprint(msg, args);
    va_end(args);
}

void File::vprint(const char *msg, va_list args) {
    if (is_null())
        return;
    vfprintf(_fp, msg, args);
}

int File::lock() {
    flock file_lock;
    file_lock.l_type = F_WRLCK;
    file_lock.l_start = 0;
    file_lock.l_whence = SEEK_SET;
    file_lock.l_len = 0;
    return fcntl(fileno(_fp), F_SETLK, &file_lock);
}

int File::unlock() {
    flock file_lock;
    file_lock.l_type = F_UNLCK;
    file_lock.l_start = 0;
    file_lock.l_whence = SEEK_SET;
    file_lock.l_len = 0;
    return fcntl(fileno(_fp), F_SETLK, &file_lock);
}

void File::flush() {
    if (is_null())
        return;
    fflush(_fp);
}

void File::close() {
    if (is_null())
        return;
    fclose(_fp);
    _fp = NULL;
}

int is_exist(const char *path) {
    return access(path, F_OK) != -1;
}

int create_dir(const char *dir) {
    if (is_exist(dir))
        return 0;
    log.debug("[FILE] Create directory: %s.", dir);
    int ret = system_call("mkdir -p %s", dir);
    if (ret != 0)
        log.error("[File] Create directory %s ERROR!", dir);
    return ret;
}

int create_file(const char *filename) {
    if (is_exist(filename))
        return 0;
    int ret = 0;
    char *dir = strbase(filename, '/');
    if (strlen(dir) > 0)
        ret = create_dir(dir);
    delete dir;
    if (ret == 0) {
        log.debug("[FILE] Create file: %s.", filename);
        ret = system_call("touch %s", filename);
        if (ret != 0) {
            log.error("[File] Create file %s ERROR!", filename);
        }
    }
    return ret;
}

int file_mode(const char *filename, int mode) {
    chmod(filename, mode);
    return 0;
}

int delete_file(const char *filename) {
    log.debug("[FILE] Delete file: %s.", filename);
    int ret = remove(filename);
    if (ret != 0)
        log.error("[FILE] Delete file %s ERROR!", filename);
    return ret;
}

int copy_file(const char *source, const char *dest) {
    log.debug("[FILE] Copy file: %s -> %s.", source, dest);
    int ret = system_call("cp %s %s", source, dest);
    if (ret != 0)
        log.error("[FILE] Copy file %s -> %s ERROR!", source, dest);
    return ret;
}

int rename_file(const char *filename, const char *newname) {
    log.debug("[FILE] Rename file: %s -> %s.", filename, newname);
    int ret = rename(filename, newname);
    if (ret != 0)
        log.error("[FILE] Rename file: %s -> %s ERROR!", filename, newname);
    return ret;
}

int gzip_compress(const char *filename) {
    log.debug("[FILE] GZIP compress file: %s.", filename);
    int ret = system_call("gzip -qf %s", filename);
    if (ret != 0)
        log.error("[FILE] Compress file %s error!", filename);
    return ret;
}
