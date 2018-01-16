/*
 * model.cpp
 *
 *  Created on: Dec 9, 2017
 *      Author: lxx
 */
#include <list>
#include "string.h"

#include "time.h"
#include "utils/log.h"
#include "utils/config.h"
#include "model.h"

// 视场的长度与宽度，也是请求天区的长宽
int length = 3;
int width = 1.5;
// 每个请求返回的最大文件数
int files_per_req = 10;
// HDD 启动时间，默认5秒
int disk_start_time = 5;
// 传输一个文件所用的时间
int trans_time_per_file = 4;

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
    disk->idle_time = 0;
    disk->disk_size = disk_size;
    disk->left_space = disk_size;
    disk->file_num = 0;

    disk->file_list = new MAP;

    disk->wt_file_list = new RW_LIST;
    disk->rd_file_list = new RW_LIST;

    return disk;
}

void del_DiskInfo(DiskInfo *disk) {
//    disk->file_list->clear();               // Maybe not need
    delete disk->file_list;
    disk->file_list = NULL;

    delete disk->rd_file_list;
    disk->rd_file_list = NULL;

    delete disk->wt_file_list;
    disk->wt_file_list = NULL;

    delete disk;
}

/*
 * 需要保证 disk 的 file_list 中一定存在 file
 * wt_file_list 中不用管
 * 将 file 复制进 disk 的 rd_file_list 中
 */
void read_file(FileInfo *file, DiskInfo *disk) {
    disk->rd_file_list->push_back(std::make_pair(trans_time_per_file, *file));
    // 启动磁盘
    if (disk->disk_state == 0) {
        disk->disk_state = 1;
    }
}

/*
 * 需要保证 file 一定可以放入 disk
 * 将 file 复制进 disk 的 wt_file_list 中
 */
void write_file(FileInfo *file, DiskInfo *disk) {
    disk->wt_file_list->push_back(std::make_pair(trans_time_per_file, *file));
    disk->left_space -= file->file_size;
    disk->file_num += 1;
    // 启动磁盘
    if (disk->disk_state == 0) {
        disk->disk_state = 1;
    }
}

/*
 * 从 disk 中删除 file
 * 可能再 file_list，也可能在 wt_file_list
 * 不需要保证 file 一定存在
 */
void delete_file(FileInfo *file, DiskInfo *disk) {
    Key key(file->ra, file->dec, file->time);

    // 从 files_list 中删除
    MAP* file_list = disk->file_list;
    MAP::iterator iter = file_list->find(key);
    if (iter != file_list->end()) {
        file_list->erase(iter);
        log.info("[DELFI] In disk %d, delete a file.", disk->disk_id);
        return;
    }

    // 否则，从 wt_file_list 中删除
    RW_LIST* wt_file_list = disk->wt_file_list;
    RW_LIST::iterator list_iter;
    for (list_iter = wt_file_list->begin(); list_iter != wt_file_list->end(); list_iter++) {
        if (file->ra == list_iter->second.ra
                && file->dec == list_iter->second.dec
                && file->time == list_iter->second.time) {
            wt_file_list->erase(list_iter); // 注意：erase()后，iter 就过期了
            log.debug("[DELFI] In disk %d, delete a file being transferring.", disk->disk_id);
            break;
        }
    }
}

/*
 * 搜索 file 是否在 disk 中
 * 既检查 file_list，又检查 wt_file_list
 * @return  0 --- not found;
 * @return  1 --- found
 * @return  2 --- half-found (Found in wt_file_list)
 */
int search_file(FileInfo *file, DiskInfo *disk) {
    Key key(file->ra, file->dec, file->time);

    // 检查是否在 file_list 里
    MAP *file_list = disk->file_list;
    MAP::iterator iter = file_list->find(key);
    if (iter != file_list->end()) {
        log.info("[SERCH] Found in disk %d.", disk->disk_id);
        return 1;
    }

    // 检查是否在 writing_file_list 里
    RW_LIST* wt_file_list = disk->wt_file_list;
    RW_LIST::iterator list_iter;
    for (list_iter = wt_file_list->begin(); list_iter != wt_file_list->end(); list_iter++) {
        if (file->ra == list_iter->second.ra
                && file->dec == list_iter->second.dec
                && file->time == list_iter->second.time) {
            log.info("[SERCH] Found in disk %d, but it is being transferring.",
                    disk->disk_id);
        }
        return 2;
    }

    return 0;
}

/*
 * 将 file 从 disk_fr 复制到 disk_to
 * 这会在整个系统中产生副本数据
 * @return  0 --- success, 1 --- failed
 */
int copy_file(FileInfo *file, DiskInfo *disk_fr, DiskInfo *disk_to) {
    // 检查 file 是否在 disk_fr
    if (search_file(file, disk_fr) == 0) {
        log.error("[CPFIL] Failed for not find source file.");
        return 1;
    }
    // 检查 file 是否在 disk_fr
    if (search_file(file, disk_to) > 0) {
        log.info("[CPFIL] Ignore this copy operation for file already existing.");
        return 0;
    }
    // 检查 disk_to 是否能装下 file
    if (disk_to->left_space < file->file_size) {
        log.error("[CPFIL] Failed for no space of disk_to.");
        return 1;
    }

    write_file(file, disk_to);

    return 0;
}

/*
 * 将 file 从 disk_fr 移动到 disk_to
 * @return  0 --- success, 1 --- failed
 */
int move_file(FileInfo *file, DiskInfo *disk_fr, DiskInfo *disk_to) {
    // 检查 file 是否在 disk_fr
    if (copy_file(file, disk_fr, disk_to) == 1) {
        log.error("[MVFIL] Failed to copy file first.");
        return 1;
    }

    // 删除原磁盘中的文件
    delete_file(file, disk_fr);

    return 0;
}

/*
 * 需要保证 file 一定可以放入 disk
 * 忽略传输时间，直接将 file 复制进 disk 的 file_list 中
 * 只用于系统初始化建立元数据索引时
 */
void add_file_init(FileInfo *file, DiskInfo *disk) {
    Key key(file->ra, file->dec, file->time);
    disk->file_list->insert(PAIR(key, *file));
    disk->left_space -= file->file_size;
    disk->file_num += 1;
}

/*
 * 将 file 按顺序自动放入 DataDisks
 * 即将其放入最后一块空闲的 dataDisk，如果满了再启动下一块
 * 只用于系统初始化建立元数据索引时
 */
int add_file(FileInfo *file) {
    // 初始化第1块disk
    if (data_disk_num == 0) {
        data_disk_array[data_disk_num] = new_DiskInfo(
                data_disk_num,
                disk_start_time,
                config.get_int("DATA", "DataDiskSize", 2000000));
        data_disk_num++;
    }
    // 当前disk空间不足
    if (data_disk_array[data_disk_num - 1]->left_space <= file->file_size) {
        if (data_disk_num >= config.get_int("DATA", "DataDiskMaxNum", 100)) {
            log.error("[MODEL] No extra DataDisks to hold more data!");
            return 1;
        }
        data_disk_array[data_disk_num] = new_DiskInfo(
                data_disk_num,
                disk_start_time,
                config.get_int("DATA", "DataDiskSize", 2000000));
        data_disk_num++;
    }

    add_file_init(file, data_disk_array[data_disk_num - 1]);

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
 * 给定请求<ra, dec, time>，判断file的质量，即两个区域公共部分的占比
 */
double file_quality(FileInfo *file, double ra, double dec, time_t start, time_t end) {
    double quality = 0.0;
    if ((ra - length < file->ra) && (file->ra < ra + length)
            && (dec - width < file->dec) && (file->dec < dec + width)
            && (start <= file->time) && (file->time <= end)) {

        double delta_length = ra - file->ra;
        if (delta_length < 0.0)
            delta_length = -delta_length;
        double common_length = length - delta_length;

        double delta_width = dec - file->dec;
        if (delta_width < 0.0)
            delta_width = -delta_width;
        double common_width = width - delta_width;

        quality = common_length * common_width / length / width;
    }
    return quality;
}

/*
 * 处理一次请求，即，找到所有相关文件，并缓存到 CacheDisk 中
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

    // 用来记录每个磁盘 涉及到的文件数 和 实际目标的文件数
    int search_files[data_disk_num];
    memset(search_files, 0, sizeof(search_files));
    int target_files[data_disk_num];
    memset(target_files, 0, sizeof(target_files));

    std::multimap<double, FileInfo> target_file_map;

    // 遍历所有DataDisk，找到所有符合条件的 file，存到 target_file_map
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

        int total_search = 0;               // 记录本磁盘检查的 file 数
        int target_file = 0;                // 记录本磁盘确认相关的 file 数

        // 遍历本磁盘上所有可能的 file，以进一步确认是否相关
        for (MAP::iterator iter = iter_low; iter != iter_up; iter++) {
            total_search++;
            double quality = file_quality(&iter->second, ra, dec, start, end);
            if (quality > 0.0) {
//                log.info("[MODEL] Find target file in disk %d", i);
//                show_file(&iter->second);
                target_file_map.insert(std::pair<double, FileInfo>(quality, iter->second));
                target_file++;
                disk->idle_time = 0;
            }
        }

        search_files[i] = total_search;
        target_files[i] = target_file;
//        log.info("[MODEL] Find %d files among %d potential files in Disk %d of total %d files.",
//                target_file, total_search, disk->disk_id, disk->file_num);
    }// END 遍历所有DataDisk，找到所有符合条件的 file，存到 target_file_map

    log.info("[MODEL] ================================================== Request Summary");
    log.info("[MODEL] Request Info: ra %9.4f  dec %9.4f  time %s", ra, dec, time);
    log.info("[MODEL] disk_id    total_files    search_files    target_files");
    for (int i = 0; i < data_disk_num; i++) {
        log.info("[MODEL] %7d    %11d    %12d    %12d",
                i, data_disk_array[i]->file_num, search_files[i], target_files[i]);
    }
    int total_files = target_file_map.size();
    log.debug("[MODEL] Find %d target files in total.", total_files);
    if (total_files > 10)
        log.debug("[MODEL] Now return the top %d correlate files.", files_per_req);
    else
        log.debug("[MODEL] Now return all target files.");

    int index = 0;
    std::multimap<double, FileInfo>::reverse_iterator rit;
    for (rit = target_file_map.rbegin(); rit != target_file_map.rend(); rit++) {
        // 处理文件
        show_file(&rit->second);


        // 只处理最相关的前 [files_per_req] 个文件
        index++;
        if (index == files_per_req)
            break;
    }

    return 0;
}

void show_file(FileInfo *file) {
    char buf[20];
    time_t2str(file->time, buf, sizeof(buf));
    log.info("[File ] file_id %d   file_size %d   ra %9.4f   dec %9.4f   time %s",
            file->file_id, file->file_size, file->ra, file->dec, buf);
}

void show_disk(DiskInfo *disk) {
    log.info("================================================== DiskInfo");
    log.info("[DiskInfo] disk_id %d   disk_state %d   idle_time %d",
            disk->disk_id, disk->disk_state, disk->idle_time);
    log.info("[DiskInfo] disk_size %d   left_space %d   file_num %d",
            disk->disk_size, disk->left_space, disk->file_num);

    log.info("-------------------------------------------------- file_list");
    MAP* file_list = disk->file_list;
    MAP::iterator iter;
    for (iter = file_list->begin(); iter != file_list->end(); iter++) {
        show_file(&iter->second);
    }
    log.info("");

    log.info("-------------------------------------------------- wt_file_list");
    RW_LIST* wt_file_list = disk->wt_file_list;
    RW_LIST::iterator wt_iter;
    for (wt_iter = wt_file_list->begin(); wt_iter != wt_file_list->end(); wt_iter++) {
        show_file(&wt_iter->second);
    }
    log.info("");

    log.info("-------------------------------------------------- rd_file_list");
    RW_LIST* rd_file_list = disk->rd_file_list;
    RW_LIST::iterator rd_iter;
    for (rd_iter = rd_file_list->begin(); rd_iter != rd_file_list->end(); rd_iter++) {
        show_file(&rd_iter->second);
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
        int state = disk->disk_state;
        // 磁盘处于关闭状态
        if (state == 0) {
            continue;
        }
        // 磁盘尚未完全开启
        else if (state > 0 && state < disk_start_time) {
            disk->disk_state++;
        }
        // 磁盘完全开启，有读写任务
        else if (disk->wt_file_list->size() > 0 || disk->rd_file_list->size() > 0) {
            // 如果磁盘已经在工作
            if (state == disk_start_time) {
            }
            // 如果磁盘空转
            else {
                disk->disk_state = disk_start_time;
            }
            // 处理读写任务

        }
        // 磁盘完全开启，无读写任务，更新busy_time
        else {
            // 如果磁盘在工作状态，标志其为空闲状态
            if (state == disk_start_time) {
                disk->disk_state++;
            }
            // 如果磁盘空转，空闲时间 +1
            else {
                if (disk->idle_time < 60) {
                    disk->idle_time++;
                } else {

                }
            }
        }
    }
}
