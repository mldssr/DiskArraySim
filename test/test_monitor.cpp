/*
 * test_monitor.cpp
 *
 *  Created on: Jan 11, 2017
 *      Author: sunchao
 */
/*  测试monitor模块，通过show_file显示是否运行正常。
 *  分为三个测试：
 *  1. 输出监控目录下现在已存在的文件。
 *  2. 在监控目录下创建yes文件，则能够输出文件信息。
 *  3. 在监控目录的子目录下创建no文件，则无信息输出。
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "utils/file.h"
#include "utils/log.h"
#include "utils/monitor.h"

void show_file(const char *filename, void *params) {
    if (filename == NULL) return;
    //assert(strcmp(filename, "yes") == 0);
    printf("File: %s\n", filename);
}

int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "Need two parameters of log path and monitor path!");
        exit(1);
    }
    char *logpath = argv[1];
    char *path = argv[2];

    printf("Initialize Log.\n");
    log.init(logpath);

    printf("Test Monitor.\n");
    Monitor monitor;
    monitor.set(path, "*", show_file, NULL);

    monitor.walk(show_file, NULL);
    printf("--------------\n");

    char *filename = new char[strlen(path) + 32];
    sprintf(filename, "%s/yes", path);
    create_file(filename);
    monitor.check();
    delete_file(filename);

    sprintf(filename, "%s/test/no", path);
    create_file(filename);
    monitor.check();
    delete_file(filename);

    printf("All tests passed!\n");
    return 0;
}



