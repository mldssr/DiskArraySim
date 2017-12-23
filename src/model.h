/*
 * model.h
 *
 *  Created on: Dec 9, 2017
 *      Author: lxx
 */

#ifndef MODEL_H_
#define MODEL_H_

#include <map>

#define MAP     std::map<Key, FileInfo>
#define PAIR    std::pair<Key, FileInfo>

struct FileInfo {
    int file_id;
    int file_size;              // MB
    double ra;                  // 赤经(Right ascension), 0 ~ 360
    double dec;                 // 赤纬(Declination), -90 ~ +90
    time_t time;
};

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

struct DiskInfo {
    int disk_id;
    int disk_state;             //  0 - 空转
                                // +n - 还需n秒才能开启
    int busy_time;              // 处理完目前的请求任务需要的时间
    int idle_time;              // 到目前为止的空转时间

    int disk_size;              // MB
    int left_space;             // MB
    int file_num;

    MAP *file_list; // Size: 48 bytes, <file_id, file>
};

FileInfo *new_FileInfo(int file_id, int file_size, double ra, double dec, time_t time);
DiskInfo *new_DiskInfo(int disk_id, int disk_state, int disk_size);
void del_DiskInfo(DiskInfo *disk);

int add_file(DiskInfo *disk, FileInfo *file);
int add_file(FileInfo *file);

/*
 * 判断file是否符合请求<ra, dec, time>的要求
 */
bool is_target_file(FileInfo *file, double ra, double dec, time_t start, time_t end);

/*
 * 处理一次请求，即，找到所有相关文件，并缓存到CacheDisk中
 * @parm time 2017-12-12
 * @return 0 - success, 1 - failed
 */
int handle_a_req(double ra, double dec, const char *time);

void show_file(FileInfo *file);
void show_disk(DiskInfo *disk);
void show_all_disks();
/*
 * 经过1s后，更新所有disk的状态
 */
void all_disks_after_1s();

extern int file_id_num;
extern int data_disk_num;
extern int cache_disk_num;
extern DiskInfo** data_disk_array;
extern DiskInfo** cache_disk_array;

#endif /* MODEL_H_ */
