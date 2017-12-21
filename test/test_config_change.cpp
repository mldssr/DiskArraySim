/*
 * config.cpp
 *
 *  Created on: 4 8, 2017
 *      Author: sunchao
 */
/* 配置模块与文件监控模块的联合测试，当配置文件改变后，配置模块会重新读取。
 *  测试方式：输出改变前后的配置。
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "utils/log.h"
#include "utils/config.h"
#include "utils/file.h"

#define LEN        256

const char *cfg_file = "test.conf";
const char *content = "[test]\nthis=this is test\n";
const char *new_content = "[test]\nthis=this\nnew=tt\n";

static int test_read() {
    config.print_all();
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Need a parameter of filename!");
        exit(1);
    }
    char *dir = argv[1];

    log.init(dir);
    File *fp = new File(stradd(dir, "/", cfg_file), "w");
    fp->print(content);
    fp->close();
    delete fp;

    config.init(stradd(dir, "/", cfg_file));
    assert(!test_read());

    printf("------\n");
    fp = new File(stradd(dir, "/", cfg_file), "w");
    fp->print(new_content);
    fp->close();
    delete fp;

    assert(!test_read());
    printf("All tests passed!\n");

    return 0;
}


