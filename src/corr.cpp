/*
 * corr.cpp
 *
 *  Created on: Jan 24, 2018
 *      Author: lxx
 */

#include <map>
#include <list>
#include "string.h"

#include "time.h"
#include "utils/basic.h"
#include "utils/log.h"
#include "utils/file.h"
#include "utils/config.h"
#include "corr.h"
//#include "model.h"

#define C_MAP    std::map<Key, std::multimap<Stats, Key>>
#define C_PAIR  std::pair<Key, std::multimap<Stats, Key>>

// 用来记录被请求的文件，以便从中分析文件相关度 <request_time, Key>
std::map<int, Key> req_file_map;
// 两个 file 能够产生关联的最大时间间隔
int time_inerval = config.get_int("CORR", "TimeInterval", 60);

// 用来存储学习到的相关度
C_MAP corrs;

/*
 * 同 file_quality()，只不过输入为 Key 而不是 FileInfo，并且不考虑时间范围
 * 即：key 与 <ra, dec> 在空间上有重合，返回重合的比例，无重合返回 0
 */
static double has_sth(Key *key0, Key *key1) {
    FileInfo *file = new_FileInfo(0, 0, key0->ra, key0->dec, key0->time);
    double ret = file_quality(file, key1->ra, key1->dec, 0, str2time_t("2020-01-01 00:00:00"));
    delete file;
    return ret;
}

/*
 * 用户调用此函数，记录下文件访问记录
 * 此函数会自动分析 之前的 files 和此 file 的关系，记录到 corrs 中
 */
void record_req_file(FileInfo *file, int exp_time) {
    // 记录最新的文件
    Key new_key(file->ra, file->dec, file->time);
    add_key(new_key);
    req_file_map.insert(std::pair<int, Key>(exp_time, new_key));

    std::map<int, Key>::iterator itlow = req_file_map.lower_bound(exp_time - time_inerval);

    // 依次检查 之前的 files 和此 file 的关系，记录到 corrs 中各自的相关文件列表中
    while (itlow != req_file_map.end()) {
        Key *key = &itlow->second;
        if (has_sth(key, &new_key) > 0) {
            add_corr_file(itlow->second, new_key);
        }
        itlow++;
    }
}

/*
 * 请求 key 后，调用此函数
 */
void add_key(Key key) {
    C_MAP::iterator iter = corrs.find(key);
    // key 不存在，创建之
    if (iter == corrs.end()) {
        std::multimap<Stats, Key> value;
        value.insert(std::pair<Stats, Key>(Stats(1, 1.0), key));
        corrs.insert(C_PAIR(key, value));
    }
    // key 存在，将其记录 +1
    else {
        // 显然，value 中 key 对应的 Stats 排在第一个
        std::multimap<Stats, Key> *tg_map = &iter->second;
        std::multimap<Stats, Key>::iterator it = tg_map->begin();
        if (it->second == key) {
            // 插入新值
            int num = it->first.hit_num + 1;
            tg_map->insert(std::pair<Stats, Key>(Stats(num, 1.0), key));
            // 删除旧值
            tg_map->erase(it);  // it 的指向并没有因插入而混乱
        } else {
            log.error("[CORR ] First element of multimap is not the key itself.");
        }
    }
}

/*
 * 请求 key 后很快请求 corr_key 时，调用此函数
 * 要保证 key 在 corrs 中一定存在，即之前调用过相应 key 的 add_key()
 * 还要保证 key ！= corr_key
 */
void add_corr_file(Key key, Key corr_key) {
    if (key == corr_key) {
        return;
    }

    C_MAP::iterator iter = corrs.find(key);
    if (iter == corrs.end()) {
        log.error("[CORR ] No entry when called add_corr_file().");
    }

    std::multimap<Stats, Key> *tg_map = &iter->second;
    std::multimap<Stats, Key>::iterator first = tg_map->begin();
    std::multimap<Stats, Key>::iterator it;
    bool has_corr_key = false;
    for (it = tg_map->begin(); it != tg_map->end(); it++) {
        if (it->second == corr_key) {
            has_corr_key = true;
            break;
        }
    }
    if (has_corr_key) {
        // 插入新值
        int num = it->first.hit_num + 1;
        double prob = (double) num / first->first.hit_num;
        tg_map->insert(std::pair<Stats, Key>(Stats(num, prob), corr_key));
        // 删除旧值
        tg_map->erase(it);
    } else {
        int num = 1;
        double prob = (double) num / first->first.hit_num;
        tg_map->insert(std::pair<Stats, Key>(Stats(num, prob), corr_key));
    }
}

/*
 * 返回 key 对应的相关文件列表，没有时返回 NULL
 * 注意：返回的列表中，第一项为 key 本身的记录，使用时应注意
 */
std::multimap<Stats, Key> *get_corr_files(Key key) {
    C_MAP::iterator iter = corrs.find(key);
    if (iter == corrs.end()) {
        log.error("[CORR ] The key hasn't been requested when called get_corr_files().");
        return NULL;
    }

    std::multimap<Stats, Key> *tg_map = &iter->second;
    if (tg_map->size() == 0) {
        log.error("[CORR ] The key has no corr files when called get_corr_files().");
        return NULL;
    }

    return tg_map;
}

/*
 * 打印 corrs
 */
void show_corrs() {
    log.info("[CORRS] ========================================================================================");
    C_MAP::iterator iter;
    for (iter = corrs.begin(); iter != corrs.end(); iter++) {
        // 打印 key
        char time_str[20];
        time_t2str(iter->first.time, time_str, 20);
        log.sublog("ra %9.4f   dec %9.4f   time %s\n", iter->first.ra, iter->first.dec, time_str);

        // 打印 value
        std::multimap<Stats, Key> *tg_map = &iter->second;
        std::multimap<Stats, Key>::iterator it;
        for (it = tg_map->begin(); it != tg_map->end(); it++) {
            char time_str[20];
            time_t2str(it->second.time, time_str, 20);
            log.sublog("    hit_num %3d   hit_prob %9.4f", it->first.hit_num, it->first.hit_prob);
            log.pure("  <====>  ra %9.4f   dec %9.4f   time %s\n", it->second.ra, it->second.dec, time_str);
        }
    }
    log.pure("\n");
}

int max_num = config.get_int("DATA", "DataDiskMaxNum", 100);
double *data_disk_hit_prob = new double[max_num]();

/*
 * 由近期内被访问的文件，推测磁盘的命中概率
 */
void cal_data_disk_hit_prob() {
    // 重置 data_disk_hit_prob
//    memset(data_disk_hit_prob, 0, max_num * sizeof(double));
    for (int i = 0; i < max_num; i++) {
        data_disk_hit_prob[i] = 0.0;
    }

    // 定位到近期的被访问文件
    std::map<int, Key>::iterator itlow = req_file_map.lower_bound(exp_time - time_inerval);

    // 从此 file 开始，依次检查各自的相关文件列表，将列表中的 hit_prob 加到对应的磁盘上
    while (itlow != req_file_map.end()) {
        // 在 corrs 中找到此 file 的对应相关文件列表
        C_MAP::iterator iter = corrs.find(itlow->second);
        if (iter == corrs.end()) {
            log.error("[CORR ] No entry when called add_corr_file().");
        }
        std::multimap<Stats, Key> *tg_map = &iter->second;
        std::multimap<Stats, Key>::iterator it;
        // 将列表中的 hit_prob 加到对应的磁盘上
        for (it = tg_map->begin(); it != tg_map->end(); it++) {
            FileInfo *file = new_FileInfo(0, 0, it->second.ra, it->second.dec, it->second.time);
            for (int i = 0; i < data_disk_num; i++) {
                if (search_file(file, data_disk_array[i]) == 1) {
                    data_disk_hit_prob[i] += it->first.hit_prob;
                }
            }
            delete file;
        }

        itlow++;
    }
}

// 日志：记录每秒钟所有磁盘的命中概率
File *track_hit_prob = NULL;

static void record_disk_hit_prob_init() {
    char *disk_hit_prob_track_file = config.get_string("TRACK", "DiskHitProbTrackFile", "disk_hit_prob_track.csv");
    track_hit_prob = new File(disk_hit_prob_track_file, "w");
    // 写入第一行
    track_hit_prob->print("time");
//    for (int i = 0; i < data_disk_num; i++) {
//        // disk_0, prob_0, disk_1, prob_1, disk_2, prob_2, ...
//        track_hit_prob->print(",disk_%d,prob_%d", i, i);
//    }
    for (int i = 0; i < data_disk_num; i++) {
        // disk_0, disk_1, disk_2, ...
        track_hit_prob->print(",disk_%d", i);
    }
    track_hit_prob->print(", ");
    for (int i = 0; i < data_disk_num; i++) {
        // prob_0, prob_1, prob_2, ...
        track_hit_prob->print(",prob_%d", i);
    }
    track_hit_prob->print("\n");
}

void record_disk_hit_prob() {
    if (track_hit_prob == NULL) {
        record_disk_hit_prob_init();
    }
    // 写入时间
    track_hit_prob->print("%d", exp_time);
    // 写入当前磁盘状态
//    for (int i = 0; i < data_disk_num; i++) {
//        track_hit_prob->print(",%d", data_disk_array[i]->disk_state);
//        track_hit_prob->print(",%f", data_disk_hit_prob[i]);
//    }
    for (int i = 0; i < data_disk_num; i++) {
        track_hit_prob->print(",%d", data_disk_array[i]->disk_state);
    }
    // 写入各个磁盘命中概率
    track_hit_prob->print(", ");
    for (int i = 0; i < data_disk_num; i++) {
        track_hit_prob->print(",%f", data_disk_hit_prob[i]);
    }
    track_hit_prob->print("\n");
}

void record_disk_hit_prob_end() {
    delete track_hit_prob;
    track_hit_prob = NULL;
}

