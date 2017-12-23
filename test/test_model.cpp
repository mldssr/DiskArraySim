/*
 * config.cpp
 *
 *  Created on: Oct 22, 2016
 *      Author: sunchao
 */
/*  测试 model 的基础功能：
 *  1. 初始化配置文件
 *  2. 以字符串输出所有配置
 *  3. 读取int型配置
 *  4. 读取double型配置
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "utils/log.h"
#include "model.h"
#include "data.h"

#define LEN        256

static int test_size() {
    DiskInfo *disk = new_DiskInfo(0, 0, 2000000);
    FileInfo *file0 = new_FileInfo(0, 500, 34, -46, 0);
    FileInfo *file1 = new_FileInfo(1, 450, 12, 1, 0);
    FileInfo *file2 = new_FileInfo(1, 450, 12, -1, 0);

    log.info("Size of file_list: %d %d", sizeof(*(disk->file_list)), disk->file_list->size());
    show_disk(disk);

    add_file(disk, file0);
    log.info("After add file0:");
    log.info("Size of file_list: %d %d", sizeof(*(disk->file_list)), disk->file_list->size());
    show_disk(disk);

    add_file(disk, file1);
    log.info("After add file1:");
    log.info("Size of file_list: %d %d", sizeof(*(disk->file_list)), disk->file_list->size());
    show_disk(disk);

    add_file(disk, file2);
    log.info("After add file2:");
    log.info("Size of file_list: %d %d", sizeof(*(disk->file_list)), disk->file_list->size());
    show_disk(disk);

    delete file0, file1, file2;
    log.info("After delete file0 file1:");
    log.info("Size of file_list: %d %d", sizeof(*(disk->file_list)), disk->file_list->size());
    show_disk(disk);

    // FIXME: 内存泄漏?
    MAP *map = disk->file_list;
    del_DiskInfo(disk);
//    map->insert(PAIR(Key(1, 4), *file0));
//    log.info("Size of file_list: %d %d", sizeof(*map), map->size());

    return 0;
}

static void test_var() {
    log.info("file_id_num: %d", file_id_num);
    log.info("data_disk_num: %d", data_disk_num);
}

static int test_handle_a_req() {
    if (scan_data("data") != 0) {
        log.error("Fail to scan data");
    }
    handle_a_req(347, -87, "2016-06-29");
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Need a parameter of log path!");
        exit(1);
    }
    char *filename = argv[1];
    log.init(filename);

//    test_size();
//    test_var();
    test_handle_a_req();
//    assert(!test_double());
//    log.info("All tests passed!\n");

    return 0;
}