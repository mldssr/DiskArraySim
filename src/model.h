/*
 * model.h
 *
 *  Created on: Dec 9, 2017
 *      Author: lxx
 */

#ifndef MODEL_H_
#define MODEL_H_

#include <map>
#include <list>

#include "req.h"

#define MAP         std::map<Key, FileInfo>
#define PAIR        std::pair<Key, FileInfo>
#define RW_LIST     std::list<std::pair<int, FileInfo>>

struct Key {
    double ra;
    double dec;
    time_t time;

    Key() : ra(0.0), dec(0.0), time(0) {
    }

    Key(double x, double y, time_t z) : ra(x), dec(y), time(z) {
    }

    bool const operator==(const Key &o) const {
        return ra == o.ra && dec == o.dec && time == o.time;
    }

    bool const operator<(const Key &o) const {
        return ra < o.ra
                || (ra == o.ra && dec < o.dec)
                || (ra == o.ra && dec == o.dec && time < o.time);
    }
};

struct FileInfo {
    int file_id;
    int file_size;              // MB
    double ra;                  // 赤经(Right ascension), 0 ~ 360
    double dec;                 // 赤纬(Declination), -90 ~ +90
    time_t time;

    int hit_count;              // 命中次数

//    Key cor_files[5];           // 相关文件列表，若本文件被命中，其相关文件的 hit_prob 增大
//    int hit_prob;               // 指示本文件被命中的概率
};

struct DiskInfo {
    int disk_id;
    int disk_state;             // -5 - 关闭（假设 HDD 启动时间为 5，定义在 model.cpp 里）
                                // -n - 还需 n 秒才能开启
                                //  0 - 正在传输数据
                                //  n - 已经空转了 n 秒

    int disk_size;              // MB
    int left_space;             // MB
    int file_num;

    int hit_count;              // 命中次数
    int start_times;            // 启动次数
    double energy;              // 总能耗

    MAP *file_list;             // Size: 48 bytes, <Key, FileInfo>

    // 正在写入的文件，与 file_list 无交集，<left_writing_time(s), file>
    RW_LIST *wt_file_list;
    // 正在读取的文件，是 file_list 的子集，<left_reading_time(s), file>，暂不使用
    RW_LIST *rd_file_list;
};

FileInfo *new_FileInfo(int file_id, int file_size, double ra, double dec, time_t time);
DiskInfo *new_DiskInfo(int disk_id, int disk_state, int disk_size);
void del_DiskInfo(DiskInfo *disk);

void read_file(FileInfo *file, DiskInfo *disk);

void write_file(FileInfo *file, DiskInfo *disk);

void delete_file(FileInfo *file, DiskInfo *disk);

int search_file(FileInfo *file, DiskInfo *disk);

int copy_file(FileInfo *file, DiskInfo *disk_fr, DiskInfo *disk_to);

int move_file(FileInfo *file, DiskInfo *disk_fr, DiskInfo *disk_to);

void add_file_init(FileInfo *file, DiskInfo *disk);

int add_file(FileInfo *file);

bool is_target_file(FileInfo *file, double ra, double dec, time_t start, time_t end);

double file_quality(FileInfo *file, double ra, double dec, time_t start, time_t end);

/*
 * 处理一次请求，即，找到所有相关文件，并缓存到CacheDisk中
 * @parm time 2017-12-12
 * @return 0 - success, 1 - failed
 */
int handle_a_req(Req *req);

void show_file(FileInfo *file);
void show_disk(DiskInfo *disk);
void show_all_disks();

void snapshot();
void snapshot_end();

void update_wt_list(DiskInfo *disk);
void update_rd_list(DiskInfo *disk);

bool time_to_shut_down();

/*
 * 经过1s后，更新所有disk的状态
 */
void all_disks_after_1s();

extern int exp_time;
extern int file_id_num;
extern int data_disk_num;
extern int cache_disk_num;
extern DiskInfo** data_disk_array;
extern DiskInfo** cache_disk_array;

#endif /* MODEL_H_ */
