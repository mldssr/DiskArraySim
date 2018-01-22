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
#include "model.h"
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
            req.gen_time = req_time;
            req.ra = ra;
            req.dec = dec;
            time_t2str((long) (day + j) * 3600 * 24, req.tg_date, 11);
            req_list.insert(R_PAIR(req.gen_time, req));
        }
    }

    File file(req_file, "w");
    file.print("gen_time,ra,dec,tg_date\n");
    R_MAP::iterator iter;
    for (iter = req_list.begin(); iter != req_list.end(); iter++) {
        file.print("%d,%f,%f,%s\n", iter->second.gen_time, iter->second.ra,
                iter->second.dec, iter->second.tg_date);
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
        int ret = sscanf(line, "%d,%lf,%lf,%s\n", &req.gen_time, &req.ra, &req.dec, req.tg_date);
        if (ret != 4) {
//            log.info("[REQ] Ignore illegal record: %s", line);
            continue;
        }
//        log.debug("[%d] %d,%f,%f,%s", ret, req.time, req.ra, req.dec, req.time_str);
        req.hand_over = false;
//        memset(req.tracks, 0, sizeof(File_track) * MaxFilesPerReq);
        for (int i = 0; i < MaxFilesPerReq; i++) {
            req.tracks[i].file_id = -1;
            req.tracks[i].res_mom = -2;
            req.tracks[i].hand_over_mom = -2;
        }
        req_list.insert(R_PAIR(req.gen_time, req));
    }

    // 输出 req_list 的内容，调试时使用
    log.debug("[req_list] ========================================================================================");
    R_MAP::iterator iter;
    for (iter = req_list.begin(); iter != req_list.end(); iter++) {
        log.debug("%d   %9.4f   %9.4f   %s", iter->second.gen_time, iter->second.ra,
                iter->second.dec, iter->second.tg_date);
        for (int i = 0; i < MaxFilesPerReq; i++) {
            File_track *track = &iter->second.tracks[i];
            if (track->file_id == -1 && track->res_mom == -2 && track->hand_over_mom == -2) {
                continue;
            }
            log.sublog("file_id %d   res_mom %d   hand_over_mom %d.\n",
                    track->file_id, track->res_mom, track->hand_over_mom);
        }
        log.pure("\n");
    }
}

/*
 * 将 req 和 file 关联起来
 */
void add_file_track(Req *req, int file_id) {
    for (int i = 0; i < MaxFilesPerReq; i++) {
        // 跳过已有的记录
        if (req->tracks[i].file_id != -1) {
            continue;
        }
        // 写入到空位上
        req->tracks[i].file_id = file_id;
        req->tracks[i].res_mom = -1;
        req->tracks[i].hand_over_mom = -1;
        break;
    }
}

/*
 * 存储系统已经成功返回 file，由此更新涉及到的请求
 * 传输文件完成时调用，此时，已经发生的请求的 tracks 应填充完毕
 */
void hand_over_a_file(int file_id) {
    R_MAP::iterator iter;
    // 逐个检查 生成时间 < 当前时间 的请求
    for (iter = req_list.begin(); iter != req_list.upper_bound(exp_time); iter++) {
        // 跳过已经处理完毕的请求
        if (iter->second.hand_over == true) {
            continue;
        }

        bool all_file_over = true;          // 已经涵盖了这个请求没有匹配文件的情况
        // 依次检查这个请求的关联文件，看是否都处理完毕
        for (int i = 0; i < MaxFilesPerReq; i++) {
            File_track *track = &iter->second.tracks[i];
            // 跳过为空的文件
            if (track->file_id == -1) {
                continue;
            }

            if (file_id == track->file_id) {
                track->hand_over_mom = exp_time;
            }

            if (track->hand_over_mom == -1) {
                all_file_over = false;
            }
        }

        if (all_file_over) {
            iter->second.hand_over = true;
        }
    }
}

// 将 req_list 中的 req 写入到 csv 文件中
void record_all_req() {
    char *req_track_file = config.get_string("TRACK", "ReqTrackFile", "req_track.csv");
    File file(req_track_file, "w");

    // 写入第一行
    file.print("gen_time,ra,dec,tg_date");
    for (int i = 0; i < MaxFilesPerReq; i++) {
        // hand_over_mom0, hand_over_mom1, hand_over_mom2...
        file.print(",hom%d", i);
    }
    file.print(",qos\n");

    // 依次写入每个请求
    R_MAP::iterator iter;
    for (iter = req_list.begin(); iter != req_list.end(); iter++) {
        // 写入基础请求信息
        file.print("%d,%f,%f,%s", iter->second.gen_time, iter->second.ra,
                iter->second.dec, iter->second.tg_date);

        // 依次写入这个请求所涉及到的文件
        bool no_file = true;    // 若所有请求都没有匹配文件，会导致这些请求的 hand_over 为 false，修复之
        int min_hand_over_mom = 0x7fffffff;
        for (int i = 0; i < MaxFilesPerReq; i++) {
            int mom = iter->second.tracks[i].hand_over_mom;
            if (mom >= 0 && mom < min_hand_over_mom) {
                min_hand_over_mom = mom;
            }
            file.print(",%d", mom);

            if (!iter->second.hand_over && iter->second.tracks[i].hand_over_mom > -2) {
                no_file = false;
            }
        }
        if (!iter->second.hand_over && no_file) {
            iter->second.hand_over = true;
        }

        // 写入 qos，-1 表示尚未完成， 0 表示未匹配到文件
        log.debug("%d %d", min_hand_over_mom, 0x7fffffff);
        log.debug("%d", iter->second.hand_over);
        int qos = min_hand_over_mom - iter->second.gen_time;
        if (!iter->second.hand_over) {
            log.debug("qos = -1");
            qos = -1;
        }
        if (iter->second.hand_over && min_hand_over_mom == 0x7fffffff) {
            log.debug("qos = 0");
            qos = 0;
        }
        file.print(",%d\n", qos);
    }
}
