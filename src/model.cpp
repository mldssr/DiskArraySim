/*
 * model.cpp
 *
 *  Created on: Dec 9, 2017
 *      Author: lxx
 */
#include "time.h"
#include "utils/log.h"
#include "utils/config.h"
#include "model.h"

// 视场的长度与宽度
int length = 3;
int width = 1.5;

// config.get_int("DATA", "DataDiskNum", 20)
// data_disk_num 记录目前所用的DataDisk数量
int file_id_num = 0;
int data_disk_num = 0;
int cache_disk_num = 0;
DiskInfo **data_disk_array = new DiskInfo*[config.get_int("DATA", "DataDiskMaxNum", 100)]();
DiskInfo **cache_disk_array = new DiskInfo*[config.get_int("DATA", "CacheDiskMaxNum", 4)]();

FileInfo *new_FileInfo(int file_id, int file_size, double ra, double dec, time_t time) {
    FileInfo *file = new FileInfo;

    file->file_id = file_id;
    file->file_size = file_size;
    file->ra = ra;
    file->dec = dec;
    file->time = time;

    return file;
}


DiskInfo *new_DiskInfo(int disk_id, int disk_state, int disk_size) {
    DiskInfo *disk = new DiskInfo;

    disk->disk_id = disk_id;
    disk->disk_state = disk_state;
    disk->busy_time = 0;
    disk->idle_time = 0;
    disk->disk_size = disk_size;
    disk->left_space = disk_size;
    disk->file_num = 0;
    disk->file_list = new MAP;

    return disk;
}

void del_DiskInfo(DiskInfo *disk){
    disk->file_list->clear();
    delete disk->file_list;
    disk->file_list = NULL;

    delete disk;
}

/*
 * 需要保证 file 一定可以放入 disk
 */
int add_file(DiskInfo *disk, FileInfo *file) {
    Key key(file->ra, file->dec, file->time);
    disk->file_list->insert(PAIR(key, *file));
    disk->left_space -= file->file_size;
    disk->file_num += 1;
    if (!disk->disk_state) {
        disk->disk_state = 1;
    }
    return 0;
}

int add_file(FileInfo *file) {
    // 初始化第1块disk
    if (data_disk_num == 0) {
        data_disk_array[data_disk_num] = new_DiskInfo(data_disk_num, 5, config.get_int("DATA", "DataDiskSize", 2000000));
        data_disk_num++;
    }
    // 当前disk空间不足
    if (data_disk_array[data_disk_num - 1]->left_space <= file->file_size) {
        if (data_disk_num >= config.get_int("DATA", "DataDiskMaxNum", 100)) {
            log.error("[MODEL] No extra DataDisks to hold more data!");
            return 1;
        }
        // 假设磁盘需要5秒启动时间
        data_disk_array[data_disk_num] = new_DiskInfo(data_disk_num, 5, config.get_int("DATA", "DataDiskSize", 2000000));
        data_disk_num++;
    }

    add_file(data_disk_array[data_disk_num - 1], file);

    return 0;
}

/*
 * 判断file是否符合请求<ra, dec, time>的要求
 */
bool is_target_file(FileInfo *file, double ra, double dec, time_t start, time_t end) {
    if ((ra - length < file->ra) && (file->ra < ra + length)
            && (dec - width < file->dec) && (file->dec < dec + width)
            && (start <= file->time) && (file->time <= end)) {
        return true;
    }
    return false;
}

/*
 * 处理一次请求，即，找到所有相关文件，并缓存到CacheDisk中
 * @parm time 2017-12-12
 * @return 0 - success, 1 - failed
 */
int handle_a_req(double ra, double dec, const char *time) {
    // 获取目标天的 time_t 范围
    char *start_time = stradd(time, " 00:00:00");
    time_t start = str2time_t(start_time);
    delete start_time;
    char *end_time = stradd(time, " 23:59:59");
    time_t end = str2time_t(end_time);
    delete end_time;

    // 遍历所有DataDisk
    for (int i = 0; i < data_disk_num; i++) {
        DiskInfo *disk = data_disk_array[i];
        if (disk == NULL)
            break;
        MAP*file_list = disk->file_list;
        // 找到磁盘中符合条件的文件的上界和下界
        Key key0 { ra - length, dec - width, end };
        MAP::iterator iter_low = file_list->upper_bound(key0);
        Key key1 { ra + length, dec + width, start };
        MAP::iterator iter_up = file_list->lower_bound(key1);

        int total_search = 0;
        int target_file = 0;
        for (MAP::iterator iter = iter_low; iter != iter_up; iter++) {
            total_search++;
            if (is_target_file(&iter->second, ra, dec, start, end)) {
                log.info("[MODEL] Find target file in disk %d", i);
                target_file++;
                show_file(&iter->second);
                disk->idle_time = 0;
            }
        }
        log.info("[MODEL] Find %d files among %d potential files in Disk %d of total %d files.",
                target_file, total_search, disk->disk_id, disk->file_num);
    }
    return 0;
}

void show_file(FileInfo *file) {
    char buf[20];
    time_t2str(file->time, buf, sizeof(buf));
    log.info("[FileInfo] file_id %d   file_size %d   ra %f   dec %f   time %s",
            file->file_id, file->file_size, file->ra, file->dec, buf);
}

void show_disk(DiskInfo *disk) {
    log.info("================================================== DiskInfo");
    log.info("[DiskInfo] disk_id %d   disk_state %d   busy_time %d   idle_time %d",
            disk->disk_id, disk->disk_state, disk->busy_time, disk->idle_time);
    log.info("[DiskInfo] disk_size %d   left_space %d   file_num %d",
            disk->disk_size, disk->left_space, disk->file_num);
    MAP*file_list = disk->file_list;
    MAP::iterator iter;
    for (iter = file_list->begin(); iter != file_list->end(); iter++) {
        show_file(&iter->second);
    }
    log.info("");
}

void show_all_disks() {
    log.debug("[MODEL] Show disk");
    for (int i = 0; i < data_disk_num; i++) {
        log.debug("[MODEL] Show disk %d", i);
        show_disk(data_disk_array[i]);
    }
}

void all_disks_after_1s() {
    DiskInfo *disk;
    for (int i = 0; i < data_disk_num; i++) {
        disk = data_disk_array[i];
        // 等待磁盘完全开启
        if (disk->disk_state > 0) {
            disk->disk_state--;
        }
        // 如果磁盘完全开启，更新busy_time
        if (disk->disk_state == 0) {
            if (disk->busy_time > 0) {
                disk->busy_time--;
            }
        }
    }
}
