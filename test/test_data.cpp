/*
 * test_data.cpp
 *
 *  Created on: Dec 12, 2017
 *      Author: lxx
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "utils/log.h"
#include "utils/config.h"
#include "utils/file.h"
#include "model.h"
#include "data.h"


void test_parse_file(const char *file_name) {
    if (parse_file(file_name) != 0) {
        log.error("Fail to parse_file");
    }
    show_all_disks();
}

void test_scan_data(const char *dir) {
    if (scan_data(dir) != 0) {
        log.error("Fail to scan data");
    }
    log.info("Find %d files in total.", file_id_num);
    log.info("Used %d data disks in total.", data_disk_num);
}

void test_footprint() {
    footprint();
}

void test_gen_file() {
    gen_file("/dev/shm/", "A_New_File.fits");
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Need a parameter of profile!");
        exit(1);
    }
    config.init(argv[1]);   // 初始化配置模块
    char *log_dir = config.get_string("LOG", "TestLogDir", "build/test_logs");
    log.init(log_dir);      // 初始化日志模块

    const char *data_name = "./test/AST3-2.obs.log.20160000.txt";
//    test_parse_file(data_name);

    test_scan_data("data");
    test_footprint();
    test_gen_file();

//    show_all_disks();
//    assert(!test_double());
//    log.info("All tests passed!\n");

    return 0;
}
