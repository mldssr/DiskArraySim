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
}

void test_scan_data(const char *dir) {
    if (scan_data(dir) != 0) {
        log.error("Fail to scan data");
    }
}

void test_footprint() {
    footprint();
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Need a parameter of profile!");
        exit(1);
    }
    config.init(argv[1]);   // 初始化配置模块
    char *log_dir = config.get_string("LOG", "TestLogDir", "build/test_logs");
    log.init(log_dir);      // 初始化日志模块

    const char *data_name = "./AST3-2.obs.log.20160000.txt";
//    test_parse_file(data_name);

    test_scan_data("data");
    test_footprint();

//    show_all_disks();
//    assert(!test_double());
//    log.info("All tests passed!\n");

    return 0;
}
