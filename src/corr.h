/*
 * corr.h
 *
 *  Created on: Jan 24, 2018
 *      Author: lxx
 */

#ifndef SRC_CORR_H_
#define SRC_CORR_H_

#include "model.h"

// 文件的统计属性，
struct Stats {
    int hit_num;
    double hit_prob;

    Stats() : hit_num(0), hit_prob(0.0) {
    }

    Stats(int x, double y) : hit_num(x), hit_prob(y) {
    }

    bool const operator==(const Stats &o) const {
        return hit_num == o.hit_num && hit_prob == o.hit_prob;
    }

    // 保证最相关的 file 排在前面
    bool const operator<(const Stats &o) const {
        return hit_num > o.hit_num;
    }
};

/*
 * 用户调用此函数，记录下文件访问记录
 * 此函数会自动分析此 file 和之前 files 的关系，记录到 corrs 中
 */
void record_req_file(FileInfo *file, int exp_time);

// 这两个接口仅为测试使用
void add_key(Key key);
void add_corr_file(Key key, Key corr_key);

/*
 * 返回 key 对应的相关文件列表，没有时返回 NULL
 * 注意：返回的列表中，第一项为 key 本身的记录，使用时应注意
 */
std::multimap<Stats, Key> *get_corr_files(Key key);

/*
 * 打印 corrs
 */
void show_corrs();

/*
 * 磁盘命中概率相关
 */
extern double *data_disk_hit_prob;
void cal_data_disk_hit_prob();
// 弃用
void record_disk_hit_prob();
void record_disk_hit_prob_end();

#endif /* SRC_CORR_H_ */
