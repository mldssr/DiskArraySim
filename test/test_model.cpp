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
#include "utils/config.h"
#include "model.h"
#include "data.h"

#define LEN        256

int test_size() {
    DiskInfo *disk = new_DiskInfo(0, 0, 2000000);
    FileInfo *file0 = new_FileInfo(0, 500, 34, -46, 0);
    FileInfo *file1 = new_FileInfo(1, 450, 12, 1, 0);
    FileInfo *file2 = new_FileInfo(1, 450, 12, -1, 0);

    log.info("Size of file_list: %d %d", sizeof(*(disk->file_list)), disk->file_list->size());
    show_disk(disk);

    add_file_init(file0, disk);
    log.info("After add file0:");
    log.info("Size of file_list: %d %d", sizeof(*(disk->file_list)), disk->file_list->size());
    show_disk(disk);

    add_file_init(file1, disk);
    log.info("After add file1:");
    log.info("Size of file_list: %d %d", sizeof(*(disk->file_list)), disk->file_list->size());
    show_disk(disk);

    add_file_init(file2, disk);
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

/*
 * 测试 read_file(), write_file(), delete_file(), search_file(), copy_file(), move_file(),
 */
int test_file_operation() {
    DiskInfo *disk0 = new_DiskInfo(0, 0, 2000000);
    DiskInfo *disk1 = new_DiskInfo(0, 0, 2000000);
    FileInfo *file0 = new_FileInfo(0, 500, 34, -46, 0);
    FileInfo *file1 = new_FileInfo(1, 450, 12, 1, 0);
    FileInfo *file2 = new_FileInfo(1, 450, 12, -1, 0);

    add_file_init(file0, disk0);
    log.info("After add file0:");
    show_disk(disk0);

    write_file(file1, disk0);
    log.info("After write_file(disk0, file1):");
    show_disk(disk0);

    int ret = search_file(file1, disk0);
    log.info("search_file(file1, disk0) returns :%d", ret);

    ret = copy_file(file1, disk0, disk1);
    log.info("copy_file(file1, disk0, disk1) returns :%d", ret);
    show_disk(disk0);
    show_disk(disk1);

    delete_file(file1, disk0);
    log.info("After delete_file(file1, disk0):");
    show_disk(disk0);

    ret = move_file(file1, disk1, disk0);
    log.info("move_file(file1, disk1, disk0) returns :%d", ret);
    show_disk(disk0);
    show_disk(disk1);

    delete file0, file1, file2;

    del_DiskInfo(disk0);
    del_DiskInfo(disk1);

    return 0;
}

void test_var() {
    log.info("file_id_num: %d", file_id_num);
    log.info("data_disk_num: %d", data_disk_num);
}

int test_handle_a_req() {
    if (scan_data("data") != 0) {
        log.error("Fail to scan data");
    }
//    handle_a_req(347, -87, "2016-06-29");
//    handle_a_req(23, -56, "2016-05-05");
    return 0;
}

int test_update_rw_list() {
    DiskInfo *disk0 = new_DiskInfo(0, -5, 2000000);
    DiskInfo *disk1 = new_DiskInfo(0, -5, 2000000);
    FileInfo *file0 = new_FileInfo(0, 500, 34, -46, 0);
    FileInfo *file1 = new_FileInfo(1, 450, 12, 1, 0);
    FileInfo *file2 = new_FileInfo(1, 450, 12, -1, 0);

    write_file(file0, disk0);
    show_disk(disk0);

    update_wt_list(disk0);
    update_rd_list(disk0);
    show_disk(disk0);

    add_file_init(file1, disk0);
    read_file(file1, disk0);
    show_disk(disk0);

    update_wt_list(disk0);
    update_rd_list(disk0);
    show_disk(disk0);

    update_wt_list(disk0);
    update_rd_list(disk0);
    show_disk(disk0);

    update_wt_list(disk0);
    update_rd_list(disk0);
    show_disk(disk0);

    update_wt_list(disk0);
    update_rd_list(disk0);
    show_disk(disk0);

    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Need a parameter of profile!");
        exit(1);
    }
    config.init(argv[1]);   // 初始化配置模块
    char *log_dir = config.get_string("LOG", "TestLogDir", "build/test_logs");
    log.init(log_dir);      // 初始化日志模块

//    test_size();
//    test_file_operation();
//    test_var();
    test_handle_a_req();
//    assert(!test_double());
//    log.info("All tests passed!\n");

//    test_update_rw_list();

    return 0;
}
