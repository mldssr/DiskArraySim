/*
 * req.cpp
 *
 *  Created on: Dec 23, 2017
 *      Author: lxx
 */
#include "string.h"
#include <map>

#include "utils/basic.h"
#include "utils/file.h"
#include "utils/log.h"
#include "utils/config.h"
#include "req.h"

// <int, Req> means <req_time, Req>
R_MAP req_list;

static int str2days(const char *time) {
    char *full_time = stradd(time, " 08:00:00");
    int days = str2time_t(full_time) / 3600 / 24;
    delete full_time;
    return days;
}

/*
 * 读取[REQ]部分参数，生成请求序列，导入到csv中
 */
void gen_req() {
    char *req_file = config.get_string("REQ", "ReqFile", "req.csv");
    char *min_time = config.get_string("REQ", "MinTime", "2016-03-14");
    char *max_time = config.get_string("REQ", "MaxTime", "2016-08-14");
    int users = config.get_int("REQ", "Users", 100);
    int max_days = config.get_int("REQ", "MaxDays", 1);
    int max_req_time = config.get_int("REQ", "MaxReqTime", 1000);

    int start_day = str2days(min_time);
    int end_day = str2days(max_time);

    for (int i = 0; i < users; i++) {
        log.debug("User %d", i);
        int days_per_user = random(1, max_days);

        int req_time = random(1, max_req_time);
        int ra = (double) random(0, 357);
        int dec = (double) random(-90, 88);
        int day = random(start_day, end_day - days_per_user + 1);

        for (int j = 0; j < days_per_user; j++) {
            log.debug("User %d, day %d", i, j);
            Req req;
            req.time = req_time;
            req.ra = ra;
            req.dec = dec;
            time_t2str((long) (day + j) * 3600 * 24, req.time_str, 11);
            req_list.insert(R_PAIR(req.time, req));
        }
    }

    File file(req_file, "w");
    file.print("req_time,ra,dec,observe_time\n");
    R_MAP::iterator iter;
    for (iter = req_list.begin(); iter != req_list.end(); iter++) {
        file.print("%d,%f,%f,%s\n", iter->second.time, iter->second.ra,
                iter->second.dec, iter->second.time_str);
    }
}

/*
 * 读取[REQ]部分参数，从csv文件中提取req，到req_list中
 */
void get_req() {
    char *req_file = config.get_string("REQ", "ReqFile", "req.csv");
    File file(req_file, "r");

    char line[100];

    // 读取表头
    file.readline(line, 100);

    while (!file.is_eof()) {
        // 读取一行记录
        // 74,276.000000,-70.000000,2016-03-14
        memset(line, 0, sizeof(line));
        file.readline(line, 100);
        // 处理完最后一行后还会进入一次循环，读取到空字符串，排除之
        if (strlen(line) == 0) {
            continue;
        }

        Req req;
        int ret = sscanf(line, "%d,%lf,%lf,%s\n", &req.time, &req.ra, &req.dec, req.time_str);
        if (ret != 4) {
//            log.info("[REQ] Ignore illegal record: %s", line);
            continue;
        }
//        log.debug("[%d] %d,%f,%f,%s", ret, req.time, req.ra, req.dec, req.time_str);
        req_list.insert(R_PAIR(req.time, req));
    }

//    R_MAP::iterator iter;
//    for (iter = req_list.begin(); iter != req_list.end(); iter++) {
//        log.debug("%d,%f,%f,%s", iter->second.time, iter->second.ra,
//                iter->second.dec, iter->second.time_str);
//    }
}
