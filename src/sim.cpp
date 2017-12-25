/*
 * sim.cpp
 *
 *  Created on: Dec 10, 2017
 *      Author: lxx
 */
#include "utils/log.h"
#include "utils/config.h"

int main(int argc, char **argv) {
    // 初始化配置文件
    if (argc >= 2) {
        config.init(argv[1]);
    } else {
        config.init("./conf.conf");
    }
    char *log_dir = config.get_string("LOG", "Directory", "build/logs");
    log.init(log_dir);      // 初始化日志模块


    return 0;
}

