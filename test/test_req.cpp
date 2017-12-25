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
#include "req.h"


int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Need a parameter of profile!");
        exit(1);
    }
    config.init(argv[1]);   // 初始化配置模块
    char *log_dir = config.get_string("LOG", "TestLogDir", "build/test_logs");
    log.init(log_dir);      // 初始化日志模块

//    gen_req();

    get_req();

//    assert(!test_double());

    return 0;
}
