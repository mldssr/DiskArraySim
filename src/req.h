/*
 * req.h
 *
 *  Created on: Dec 23, 2017
 *      Author: lxx
 */

#ifndef SRC_REQ_H_
#define SRC_REQ_H_

#define MaxFilesPerReq 10       // 每个请求返回的最大文件数

struct File_track {
    int file_id;                // 表示此实例不跟踪文件，默认值-1
    int res_mom;                // response moment，开始处理的时刻，默认值-2，待更新为-1
    int hand_over_mom;          // 处理完毕的时刻，默认值-2，待更新为-1
};

struct Req {
    int gen_time;               // 请求生成时间
    double ra;
    double dec;
    char tg_date_start[11];          // target_date, 例如 "2017-03-11"，从 00:00:00 起算
    char tg_date_end[11];            // target_date, 例如 "2017-03-11"，到 23:59:59 截止

    // 记录 Req 所涉及的 file 的处理情况
    bool hand_over;             // 是否已经处理完毕
    File_track tracks[MaxFilesPerReq];
};

#define R_MAP     std::multimap<int, Req>
#define R_PAIR    std::pair<int, Req>

extern R_MAP req_list;

void gen_req();

void get_req();

void add_file_track(Req *req, int file_id);

void hand_over_a_file(int file_id);

void record_all_req();

#endif /* SRC_REQ_H_ */
