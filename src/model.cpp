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
#include "utils/file.h"
#include "utils/config.h"
#include "model.h"
#include "corr.h"
//#include "req.h"

// 实验中的模拟计时，从 0 开始，每秒加 1
int exp_time = 0;

// 视场的长度与宽度，也是请求天区的长宽
int length = 3;
int width = 1.5;
// HDD 启动时间，默认5秒
int disk_start_time = 9;
// 传输一个文件所用的时间
int trans_time_per_file = 4;

// config.get_int("DATA", "DataDiskNum", 20)
// data_disk_num 记录目前所用的DataDisk数量
int file_id_num = 0;
int data_disk_num = 0;
int cache_disk_num = 0;
DiskInfo **data_disk_array = NULL;
DiskInfo **cache_disk_array = NULL;
// FIXME: not work
//cache_disk_array = new DiskInfo*[config.get_int("DATA", "CacheDiskMaxNum", 4)]();

FileInfo *new_FileInfo(int file_id, int file_size, double ra, double dec, time_t time) {
    FileInfo *file = new FileInfo;

    file->file_id = file_id;
    file->file_size = file_size;
    file->ra = ra;
    file->dec = dec;
    file->time = time;
    file->hit_count = 0;

    return file;
}


DiskInfo *new_DiskInfo(int disk_id, int disk_state, int disk_size) {
    DiskInfo *disk = new DiskInfo;

    disk->disk_id = disk_id;
    disk->disk_state = disk_state;
    disk->disk_size = disk_size;
    disk->left_space = disk_size;
    disk->file_num = 0;
    disk->hit_count = 0;
    disk->start_times = 0;
    disk->energy = 0.0;
    disk->prob_rank = -1;
    disk->idle_th = config.get_int("MAIN", "MaxIdleTime", 60);
    disk->delayed_time = 0;
    disk->delayed_time_th = 10;

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
    file->hit_count++;
    disk->hit_count++;
    disk->rd_file_list->push_back(std::make_pair(trans_time_per_file, *file));
    // 启动磁盘
//    if (disk->disk_state == 0 - disk_start_time) {
//        disk->disk_state++;
//    }
}

/*
 * 需要保证 file 一定可以放入 disk
 * 将 file 复制进 disk 的 wt_file_list 中
 */
void write_file(FileInfo *file, DiskInfo *disk) {
    disk->wt_file_list->push_back(std::make_pair(trans_time_per_file, *file));
    disk->wt_file_list->back().second.hit_count = 0;        // 从头计数
    disk->left_space -= file->file_size;
    disk->file_num += 1;
    // 启动磁盘
//    if (disk->disk_state == 0 - disk_start_time) {
//        disk->disk_state++;
//    }
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
//        log.info("[SERCH] Found in disk %d.", disk->disk_id);
        return 1;
    }

    // 检查是否在 writing_file_list 里
    RW_LIST* wt_file_list = disk->wt_file_list;
    RW_LIST::iterator list_iter;
    for (list_iter = wt_file_list->begin(); list_iter != wt_file_list->end(); list_iter++) {
        if (file->ra == list_iter->second.ra
                && file->dec == list_iter->second.dec
                && file->time == list_iter->second.time) {
//            log.info("[SERCH] Found in disk %d, but it is being transferring.",
//                    disk->disk_id);
        }
        return 2;
    }

    return 0;
}

/*
 * 搜索 file 在哪些 DataDisk 中，返回最佳的 disk_id
 * 最佳应考虑各个磁盘的相应时间，磁盘状态
 * 只检查 file_list，不检查 wt_file_list
 * @return  -1 --- not found;
 * @return   n --- 最佳的 disk_id
 */
int search_all_disks(FileInfo *file) {
    // 统计可选方案
    bool has_target[data_disk_num];
    int disk_tasks[data_disk_num];
    int disk_states[data_disk_num];
    for (int i = 0; i < data_disk_num; i++) {
        // [IF] 实实在在找到
        if (search_file(file, data_disk_array[i]) == 1) {
            has_target[i] = true;
        } else {
            has_target[i] = false;
        }

        int rd_size = data_disk_array[i]->rd_file_list->size();
        int wt_size = data_disk_array[i]->wt_file_list->size();
        disk_tasks[i] = rd_size + wt_size;

        disk_states[i] = data_disk_array[i]->disk_state;
    }

    // 从开着的（包括正在启动的）disk 中找到最不忙的
    int min_tasks = 10000;
    int disk_id = -1;
    for (int i = 0; i < data_disk_num; i++) {
        if (has_target[i] && disk_states[i] > 0 - disk_start_time) {
            if (disk_tasks[i] < min_tasks) {
                disk_id = i;
                min_tasks = disk_tasks[i];
            }
        }
    }
    if (min_tasks < 10000) {
//        log.debug("[SR_AL] Found in opened disk %d.", disk_id);
        return disk_id;
    }

    // 从关着的 disk 中找到最忙的
    int max_tasks = -1;
    disk_id = -1;
    for (int i = 0; i < data_disk_num; i++) {
        if (has_target[i] && disk_states[i] == 0 - disk_start_time) {
            if (disk_tasks[i] > max_tasks) {
                disk_id = i;
                max_tasks = disk_tasks[i];
            }
        }
    }
    if (max_tasks > -1) {
//        log.debug("[SR_AL] Found in closed disk %d.", disk_id);
        return disk_id;
    }

    return -1;
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
    // 检查 file 是否在 disk_to
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
    // 首次调用时初始化全局变量
    if (data_disk_array == NULL) {
        data_disk_array = new DiskInfo*[config.get_int("DATA", "DataDiskMaxNum", 100)]();
    }
    // 初始化第1块disk
    if (data_disk_num == 0) {
        data_disk_array[data_disk_num] = new_DiskInfo(
                data_disk_num,
                0 - disk_start_time,
                config.get_int("DATA", "DataDiskSize", 2000000));
        data_disk_num++;
    }
    // 当前 disk 剩余空间不足预留空间
    int data_disk_preserved_space = config.get_int("DATA", "DataDiskPreservedSpace", 200000);
    if (data_disk_array[data_disk_num - 1]->left_space <= data_disk_preserved_space) {
        if (data_disk_num >= config.get_int("DATA", "DataDiskMaxNum", 100)) {
            log.error("[MODEL] No extra DataDisks to hold more data!");
            return 1;
        }
        data_disk_array[data_disk_num] = new_DiskInfo(
                data_disk_num,
                0 - disk_start_time,
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
int handle_a_req(Req *req) {
    // 获取目标天的 time_t 范围
    char *start_date = stradd(req->tg_date_start, " 00:00:00");
    time_t start = str2time_t(start_date);
    delete start_date;
    char *end_date = stradd(req->tg_date_end, " 23:59:59");
    time_t end = str2time_t(end_date);
    delete end_date;

    // 用来记录每个磁盘 涉及到的文件数 和 实际目标的文件数
    int search_files[data_disk_num];
    memset(search_files, 0, sizeof(search_files));
    int target_files[data_disk_num];
    memset(target_files, 0, sizeof(target_files));

    // 遍历所有DataDisk，找到所有符合条件的 file，存到 target_file_map
    std::multimap<double, FileInfo> target_file_map;        // <file_quality, File>
    for (int i = 0; i < data_disk_num; i++) {
        DiskInfo *disk = data_disk_array[i];
        if (disk == NULL)
            break;
        MAP*file_list = disk->file_list;
        // 找到磁盘中符合条件的文件的上界和下界
        Key key0 { req->ra - length, req->dec - width, end };
        MAP::iterator iter_low = file_list->upper_bound(key0);
        Key key1 { req->ra + length, req->dec + width, start };
        MAP::iterator iter_up = file_list->lower_bound(key1);

        int total_search = 0;               // 记录本磁盘检查的 file 数
        int target_file = 0;                // 记录本磁盘确认相关的 file 数

        // 遍历本磁盘上所有可能的 file，以进一步确认是否相关
        for (MAP::iterator iter = iter_low; iter != iter_up; iter++) {
            total_search++;
            double quality = file_quality(&iter->second, req->ra, req->dec, start, end);
            if (quality > 0.0) {
//                log.info("[MODEL] Find target file in disk %d", i);
//                show_file(&iter->second);
                target_file_map.insert(std::pair<double, FileInfo>(quality, iter->second));
                target_file++;
            }
        }

        search_files[i] = total_search;
        target_files[i] = target_file;
//        log.info("[MODEL] Find %d files among %d potential files in Disk %d of total %d files.",
//                target_file, total_search, disk->disk_id, disk->file_num);
    }// END 遍历所有DataDisk，找到所有符合条件的 file，存到 target_file_map

    // 记录这次 req 的处理情况
    log.info("[MODEL] ======================================== Request Summary ========");
    log.sublog("        Request Info: gen_time %5d  ra %9.4f  dec %9.4f  date %s ~ %s\n",
            req->gen_time, req->ra, req->dec, req->tg_date_start, req->tg_date_end);
//    log.sublog("        disk_id    total_files    search_files    target_files\n");
//    for (int i = 0; i < data_disk_num; i++) {
//        log.sublog("        %7d    %11d    %12d    %12d\n",
//                i, data_disk_array[i]->file_num, search_files[i], target_files[i]);
//    }
    int total_files = target_file_map.size();
    log.sublog("        Find %d target files in total.\n", total_files);

    if (total_files > MaxFilesPerReq)
        log.sublog("        Now return the top %d correlate files.\n", MaxFilesPerReq);
    else if (total_files > 0)
        log.sublog("        Now return all %d target files.\n", total_files);
    else
        log.sublog("        No file to return.\n");

    // 处理相应文件
    int index = 0;
    std::multimap<double, FileInfo>::reverse_iterator rit;
    for (rit = target_file_map.rbegin(); rit != target_file_map.rend(); rit++) {
        // 处理文件
        int disk_id = search_all_disks(&rit->second);
        read_file(&rit->second, data_disk_array[disk_id]);
        add_file_track(req, rit->second.file_id);

//        log.pure("           [%2d] quality: %f, diskID: %2d", index, rit->first, disk_id);
//        show_file(&rit->second);

        // 记录到 corr 模块
        record_req_file(&rit->second, exp_time);

        // 只处理最相关的前 [MaxFilesPerReq] 个文件
        index++;
        if (index == MaxFilesPerReq)
            break;
    }

    return 0;
}

void show_file(FileInfo *file) {
    char buf[20];
    time_t2str(file->time, buf, sizeof(buf));
    log.sublog("[File ] file_id %6d   file_size %d   ra %9.4f   dec %9.4f   time %s   hit_count %d\n",
            file->file_id, file->file_size, file->ra, file->dec, buf, file->hit_count);
}

void show_disk(DiskInfo *disk) {
    log.info("[DiskInfo] ========================================================================================");
    int permill = disk->left_space * 1000 / disk->disk_size;
    log.sublog("[DiskInfo] id %d   state %d   size %d   left_space %d (%d‰)   file_num %d\n",
            disk->disk_id, disk->disk_state, disk->disk_size, disk->left_space, permill, disk->file_num);

    log.sublog("-------------------------------------------------- file_list --------\n");
    MAP* file_list = disk->file_list;
    MAP::iterator iter;
    for (iter = file_list->begin(); iter != file_list->end(); iter++) {
        show_file(&iter->second);
    }
    log.pure("\n");

    log.sublog("-------------------------------------------------- wt_file_list --------\n");
    RW_LIST* wt_file_list = disk->wt_file_list;
    RW_LIST::iterator wt_iter;
    for (wt_iter = wt_file_list->begin(); wt_iter != wt_file_list->end(); wt_iter++) {
        log.sublog("[%d]\n", wt_iter->first);
        show_file(&wt_iter->second);
    }
    log.pure("\n");

    log.sublog("-------------------------------------------------- rd_file_list --------\n");
    RW_LIST* rd_file_list = disk->rd_file_list;
    RW_LIST::iterator rd_iter;
    for (rd_iter = rd_file_list->begin(); rd_iter != rd_file_list->end(); rd_iter++) {
        log.sublog("[%d]\n", rd_iter->first);
        show_file(&rd_iter->second);
    }
    log.pure("\n");
}

void show_all_disks() {
    log.debug("[MODEL] Show disk");
    for (int i = 0; i < data_disk_num; i++) {
        log.debug("[MODEL] Show disk %d", i);
        show_disk(data_disk_array[i]);
    }
}

// 日志：记录每秒钟所有磁盘的状态
File *shot = NULL;

static void snapshot_init() {
    char *snap_shot_file = config.get_string("TRACK", "SnapshotFile", "./track/snapshot.csv");
    shot = new File(snap_shot_file, "w");
    // 写入第一行
    shot->print("time");
    for (int i = 0; i < data_disk_num; i++) {
        // disk_0, disk_1, disk_2, ...
        shot->print(",disk_%d", i);
    }
    shot->print(",opened");
    shot->print(", ");
    for (int i = 0; i < data_disk_num; i++) {
        // tasks_0, tasks_1, tasks_2, ...
        shot->print(",tasks_%d", i);
    }
    if (config.get_int("MAIN", "Mode", 0) == 0) {       // 普通模式下不需要记录命中指数部分
        shot->print("\n");
        return;
    }
    shot->print(", ");
    for (int i = 0; i < data_disk_num; i++) {
        // prob_0, prob_1, prob_2, ...
        shot->print(",prob_%d", i);
    }
    shot->print("\n");
}

int total_opened = 0;
void snapshot() {
    if (shot == NULL) {
        snapshot_init();
    }
    // 写入时间
    shot->print("%5d", exp_time);
    // 写入当前各个磁盘状态
    int opened = 0;
    for (int i = 0; i < data_disk_num; i++) {
        shot->print(",%2d", data_disk_array[i]->disk_state);
        if (data_disk_array[i]->disk_state >= 0) {
            opened++;
        }
    }
    shot->print(",%2d", opened);
    total_opened += opened;
    // 写入当前各个磁盘任务数
    shot->print(", ");
    for (int i = 0; i < data_disk_num; i++) {
        int tasks = data_disk_array[i]->rd_file_list->size() + data_disk_array[i]->wt_file_list->size();
        shot->print(",%d", tasks);
    }
    if (config.get_int("MAIN", "Mode", 0) == 0) {       // 普通模式下不需要记录命中指数部分
        shot->print("\n");
        return;
    }
    // 写入各个磁盘命中概率
    shot->print(", ");
    for (int i = 0; i < data_disk_num; i++) {
        shot->print(",%8.2f", data_disk_hit_prob[i]);
    }
    shot->print("\n");
}

void snapshot_end() {
    delete shot;
    shot = NULL;

    int total_start_times = 0;
    double total_energy = 0.0;
    File disk_stat("./track/disk_stat.txt", "w");
    disk_stat.print("disk_id,   hit_count,   start_times,     energy\n");
    for (int i = 0; i < data_disk_num; i++) {
        DiskInfo *disk = data_disk_array[i];
        disk_stat.print("%7d,   %9d,   %11d,   %8.1f\n",
                i, disk->hit_count, disk->start_times, disk->energy);
        total_start_times += disk->start_times;
        total_energy += disk->energy;
    }

    disk_stat.print("\n");
    disk_stat.print("Total start times: %d.\n", total_start_times);
    disk_stat.print("Average start times per disk: %f.\n", 1.0 * total_start_times / data_disk_num);
    disk_stat.print("\n");
    disk_stat.print("Total energy consumed: %f kJ.\n", total_energy / 1000);
    disk_stat.print("Average energy consumed per second: %f J.\n", total_energy / exp_time);
    disk_stat.print("\n");
    disk_stat.print("Average opened disks per second: %f.\n", 1.0 * total_opened / exp_time);
}

void update_wt_list(DiskInfo *disk) {
    RW_LIST::iterator iter = disk->wt_file_list->begin();
    if (iter != disk->wt_file_list->end()) {
        // 传输 1 秒数据
        if (iter->first > 0) {
            iter->first--;
        }
        // 若传输完毕，将 file 从 wt_list 移动到 file_list
        if (iter->first == 0) {
            FileInfo *file = &iter->second;
            Key key(file->ra, file->dec, file->time);
            disk->file_list->insert(PAIR(key, *file));
            disk->wt_file_list->erase(iter);
        }
    }
}

void update_rd_list(DiskInfo *disk) {
    RW_LIST::iterator iter = disk->rd_file_list->begin();
    if (iter != disk->rd_file_list->end()) {
        // 传输 1 秒数据
        if (iter->first > 0) {
            iter->first--;
        }
        // 若传输完毕，将 file 从 rd_list 直接删除
        if (iter->first == 0) {
            hand_over_a_file(iter->second.file_id);
            disk->rd_file_list->erase(iter);
        }
    }
}

static void update_rw_list(DiskInfo *disk) {
    update_wt_list(disk);
    update_rd_list(disk);
}

/* 不管磁盘状态 */
static inline bool not_busy(DiskInfo *disk) {
    return disk->rd_file_list->empty() && disk->wt_file_list->empty();
}

/*
 * 所有磁盘没有读写任务，且都已经关闭
 */
bool time_to_shut_down() {
    bool ret = true;
    // 若有磁盘 有任务 或者 无任务但尚未关闭，返回 false
    for (int i = 0; i < data_disk_num; i++) {
        if (!not_busy(data_disk_array[i]) || data_disk_array[i]->disk_state != 0 - disk_start_time) {
            ret = false;
        }
    }
    return ret;
}
/*
 * WD10EZEX
 * Power Management
 * 12VDC ±10% (A, peak)             2.5
 * Average power requirements (W)
 * Read/Write                       6.8
 * Idle                             6.1
 * Standby/Sleep                    1.2
 */
static double get_power(int state) {
    double power = 0.0;
    if (state == 0 - disk_start_time) {     // 关闭
        power = 0.0;
    } else if (state < 0) {                 // 启动中
        power = 30.0;
    } else if (state == 0) {                // Read/Write
        power = 6.8;
    } else {                                // Idle
        power = 6.1;
    }
    return power;
}

static double get_total_power() {
    double total_power = 0.0;
    for (int i = 0; i < data_disk_num; ++i) {
        total_power += get_power(data_disk_array[i]->disk_state);
    }
    return total_power;
}

static void update_th() {
    // 更新所有磁盘的 prob_rank
    for (int i = 0; i < data_disk_num; i++) {
        // 统计有多少 disk 的命中指数比 disk i 高，其 prob_rank 就是几
        int rank = 0;
        for (int j = 0; j < data_disk_num; j++) {
            if (data_disk_hit_prob[j] > data_disk_hit_prob[i]) {
                rank++;
            }
        }
        data_disk_array[i]->prob_rank = rank;
    }

    double max_th = config.get_int("MAIN", "MaxIdleTime", 60) * 2;
    double min_th = config.get_int("MAIN", "MaxIdleTime", 60) * 0.5;
    double step = (max_th - min_th) / (data_disk_num + 1);
    for (int i = 0; i < data_disk_num; i++) {
        DiskInfo *disk = data_disk_array[i];
        disk->idle_th = (int)(min_th + (data_disk_num - disk->prob_rank) * step);
//        disk->delayed_time_th = 1 + 0.1 * disk->prob_rank;
//        disk->idle_th = 1;
        disk->delayed_time_th = 0;
    }
}

static int rw_tasks (DiskInfo *disk) {
    return disk->rd_file_list->size() + disk->wt_file_list->size();
}

/* 找到所有未通电磁盘中最繁忙的 */
static int urgent_disk() {
    int id = -1;
    int max_tasks = 0;
    for (int i = 0; i < data_disk_num; ++i) {
        if (data_disk_array[i]->disk_state == 0 - disk_start_time) {
            int tasks = rw_tasks(data_disk_array[i]);
            if (tasks > max_tasks) {
                max_tasks = tasks;
                id = i;
            }
        }
    }
    return id;
}

double total_power = 0.0;
bool power_overflow = false;
int urgent_disk_id = -1;        // 满荷时，最想启动却启动不了的磁盘

/* 用来更新以上三个变量 */
static void update_urgent_disk() {
    total_power = get_total_power();
    if (total_power > config.get_int("MAIN", "MaxPower", 500)) {
        urgent_disk_id = urgent_disk();
        power_overflow = true;
    } else {
        urgent_disk_id = -1;
        power_overflow = false;
    }
}

/* 找到已开启磁盘中最空闲的那个 */
static int most_idle_disk() {
    int id = -1;
    int max_state = 0 - disk_start_time;
    for (int i = 0; i < data_disk_num; ++i) {
        int state = data_disk_array[i]->disk_state;
        if (state > max_state) {
            max_state = state;
            id = i;
        }
    }
    return id;
}

void all_disks_after_1s() {
    int mode = config.get_int("MAIN", "Mode", 0);
    if (mode == 1 && exp_time >= 100 && exp_time % 10 == 0) {
        update_th();
    }

    DiskInfo *disk;
    for (int i = 0; i < data_disk_num; i++) {
        update_urgent_disk();
        disk = data_disk_array[i];
        int state = disk->disk_state;
        disk->energy += get_power(state);
        // 磁盘处于关闭状态，无读写任务
        if (state == 0 - disk_start_time && not_busy(disk)) {
        }
        // 磁盘处于关闭状态，有读写任务，则启动磁盘
        else if (state == 0 - disk_start_time && !not_busy(disk)) {
            // 满荷时不再启动额外磁盘
            if (power_overflow) {
                log.debug("[MODEL] Not launch disk %d for power overflow: %f", disk->disk_id, total_power);
                continue;
            }
            // 读写任务少时延迟启动
            if (mode == 1 && exp_time > 100 && rw_tasks(disk) < 2) {
                disk->delayed_time++;
                if (disk->delayed_time >= disk->delayed_time_th) {
                    disk->disk_state++;
                    disk->start_times++;
                    disk->delayed_time = 0;
//                    log.debug("[MODEL] Start disk %d", disk->disk_id);
                }
                continue;
            }
            disk->disk_state++;
            disk->start_times++;
//            log.debug("[MODEL] Start disk %d", disk->disk_id);
        }
        // 磁盘尚未完全开启
        else if (state < 0) {
            disk->disk_state++;
        }
        // 磁盘完全开启，有读写任务
        else if (!not_busy(disk)) {
            // 如果磁盘已经在工作
            if (state == 0) {
            }
            // 如果磁盘空转，将其置为忙碌状态
            else {
                disk->disk_state = 0;
            }
            // 处理读写任务
            update_rw_list(disk);
        }
        // 磁盘完全开启，无读写任务，更新 disk_state (busy_time)
        else {
            // 如果磁盘在工作状态，标志其为空闲状态
            if (state == 0) {
                disk->disk_state++;
            }
            // 如果磁盘空转，空闲时间 +1
            else {
                // 满荷时有启动新磁盘的需求，立即关闭当前磁盘，省出 power 来
//                if (power_overflow && urgent_disk_id >= 0 && mode == 0) {
//                    disk->disk_state = 0 - disk_start_time;
////                    log.debug("[MODEL] Close disk %d to save power", disk->disk_id);
//                }
                if (disk->disk_state < disk->idle_th) {
                    disk->disk_state++;
                }
                // 超过阈值，关闭磁盘
                else {
                    disk->disk_state = 0 - disk_start_time;
//                    log.debug("[MODEL] CLose disk %d", disk->disk_id);
                }
            }
        }
    }
}
