/*
 * test_corr.cpp
 *
 *  Created on: Jan 24, 2018
 *      Author: lxx
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "utils/basic.h"
#include "utils/log.h"
#include "utils/config.h"
#include "model.h"
#include "data.h"
#include "corr.h"

Key key0(0, 1, 3600);
Key key1(2, 3, 7200);
Key key2(4, 5, 10800);

void test_add_corr() {
    for (int i = 0; i < 10; i++) {
        add_key(key0);
    }
    for (int i = 0; i < 20; i++) {
        add_key(key1);
    }
    for (int i = 0; i < 30; i++) {
        add_key(key2);
    }
    for (int i = 0; i < 5; i++) {
        add_corr_file(key0, key1);
    }
    for (int i = 0; i < 6; i++) {
        add_corr_file(key0, key2);
    }
    for (int i = 0; i < 9; i++) {
        add_corr_file(key2, key1);
    }

    show_corrs();

    std::multimap<Stats, Key> *tg_map = get_corr_files(key0);
    if (tg_map != NULL) {
        std::multimap<Stats, Key>::iterator it;
        for (it = tg_map->begin(); it != tg_map->end(); it++) {
            char time_str[20];
            time_t2str(it->second.time, time_str, 20);
            log.sublog("    hit_num %3d   hit_prob %9.4f", it->first.hit_num, it->first.hit_prob);
            log.pure("  <====>  ra %9.4f   dec %9.4f   time %s\n", it->second.ra, it->second.dec, time_str);
        }
    }
}


FileInfo *file0 = new_FileInfo(0, 200, 1.0, 2.0, 0);
FileInfo *file1 = new_FileInfo(0, 200, 1.1, 2.1, 1);
FileInfo *file2 = new_FileInfo(0, 200, 1.2, 2.2, 2);

void test_record_req_file() {
    record_req_file(file0, 1000);
    show_corrs();

    record_req_file(file1, 1001);
    show_corrs();

    record_req_file(file2, 2000);
    show_corrs();
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Need a parameter of profile!");
        exit(1);
    }
    config.init(argv[1]);   // 初始化配置模块
    char *log_dir = config.get_string("LOG", "TestLogDir", "build/test_logs");
    log.init(log_dir);      // 初始化日志模块

//    test_add_corr();
    test_record_req_file();

    return 0;
}


