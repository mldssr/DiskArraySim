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


int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Need a parameter of log path!");
        exit(1);
    }
    char *filename = argv[1];
    log.init(filename);

    const char *data_name = "./AST3-2.obs.log.20160000.txt";
//    test_parse_file(data_name);

    test_scan_data("data");

    show_all_disks();
//    assert(!test_double());
//    log.info("All tests passed!\n");

    return 0;
}
