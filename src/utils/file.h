/*
 * file.h
 *
 *  Created on: Dec 20, 2016
 *      Author: sunchao
 */

#ifndef FILE_H_
#define FILE_H_

#include <stdio.h>
#include "basic.h"

class File {
private:
    FILE *_fp;
public:
    File(const char *filename, const char *modes);
    File(const char *dir, const char *filename, const char *modes);
    ~File();

    /* 文件指针是否为空(文件打开错误)。 */
    bool is_null();
    /* 文件指针是否为空(文件打开错误)或者文件到达结尾。 */
    bool is_eof();
    /* 文件大小。读取完文件大小，恢复文件指针的当前位置。 */
    size_t size();
    /* 设置文件指针的位置。 */
    void seek(long pos);
    /* 读取文件。从文件中读取count个大小为size的数据到data中。 */
    void read(void *data, size_t size, size_t count);
    /* 读取一行字符 */
    void readline(char *data, size_t size);
    /* 写入文件。从data中读取count个大小为size的数据写入到文件中。 */
    void write(const void *data, size_t size, size_t count);
    /* 从文件读取数据。 */
    int scan(const char *msg, ...);
    /* 写入文件。 */
    void print(const char *msg, ...);
    /* 写入文件。 */
    void vprint(const char *msg, va_list args);
    /* flush文件。 */
    void flush();
    /* 锁定文件。 */
    int lock();
    /* 解锁文件。 */
    int unlock();
    /* 关闭文件。 */
    void close();
};

/* 判断文件或文件夹是否存在。1为存在，0为不存在。 */
int is_exist(const char *path);
/* 创建文件夹。 */
int create_dir(const char *dir);
/* 创建文件。 */
int create_file(const char *filename);
/* 修改文件权限。 */
int file_mode(const char *filename, int mode);
/* 删除文件。 */
int delete_file(const char *filename);
/* 创建文件。 */
int copy_file(const char *source, const char *dest);
/* 文件改名。 */
int rename_file(const char *filename, const char *newname);
/* 以GZIP格式压缩文件。 */
int gzip_compress(const char *filename);

#endif /* SYSTEM_H_ */
