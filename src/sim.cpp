/*
 * sim.cpp
 *
 *  Created on: Dec 10, 2017
 *      Author: lxx
 */
#include "utils/log.h"
#include "utils/config.h"
#include "model.h"
#include "data.h"
#include "req.h"
#include "corr.h"

int main(int argc, char **argv) {
    // 初始化配置文件
    if (argc >= 2) {
        config.init(argv[1]);
    } else {
        config.init("./conf.conf");
    }
    char *log_dir = config.get_string("LOG", "Directory", "./track/logs");
    log.init(log_dir);      // 初始化日志模块

    // 扫描数据
    const char *dir = config.get_string("DATA", "Dir", "data");
    if (scan_data(dir) != 0) {
        log.error("Fail to scan data");
    }
    footprint();

    // 获取请求
    get_req();

    // 运行
    int max_req_time = config.get_int("REQ", "MaxReqTime", 1000);
    R_MAP::iterator iter = req_list.begin();
    while((exp_time < max_req_time + 1) || (!time_to_shut_down())) {
//        log.debug("exp_time: %d", exp_time);
        // 处理这一秒的所有请求
        while (iter->second.gen_time == exp_time) {
            handle_a_req(&iter->second);
            iter++;
            if (iter != req_list.end()) {
                log.debug("[MAIN ] Next req's gen_time: %d", iter->second.gen_time);
            }
        }

        all_disks_after_1s();
        cal_data_disk_hit_prob();
//        record_disk_hit_prob();
        snapshot();
        exp_time++;
    }

    record_all_req();

    snapshot_end();
//    record_disk_hit_prob_end();

    show_corrs();

    return 0;
}

