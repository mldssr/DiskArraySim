/*
 * disk_ctl.cpp
 *
 *  Created on: Nov 17, 2018
 *      Author: lxx
 */
#include "pthread.h"
#include "unistd.h"
#include <stdio.h>

#include "utils/basic.h"
#include "utils/file.h"
#include "utils/config.h"
#include "utils/log.h"
#include "model.h"
#include "data.h"
#include "disk_ctl.h"

// 用于处理实际 I/O

struct Args {
    int id;  // 磁盘编号
};

Args **argss = NULL;
char **dev = NULL;                  // 设备名，例如 /dev/sdb
char **dir = NULL;                  // 目录名，例如 /media/hdd00/fits/
const char *tmp_dir = "/dev/shm/";  // 临时内存目录，读写缓冲区

// 处理写队列
// @parm id 磁盘ID
void handle_wt(int id) {
    DiskInfo *disk = data_disk_array[id];
    log.debug("[DISK ] Thread %d: Going to handle %d file writes.", id, disk->wt_file_list->size());
    RW_LIST::iterator iter = disk->wt_file_list->begin();
    while (iter != disk->wt_file_list->end()) {
        // 传输数据
        FileInfo *file = &iter->second;
        char file_name[100];
        if (get_file_name(file, file_name, sizeof(file_name))) {    // 解析文件名失败
            log.error("[DISK ] Thread %d: Fail to get file_name.", id);
            continue;
        }
        char path[100] = {0}; // 足够大，例如 /media/hdd00/fits/332.4060_-56.6292_2016-03-14_17:41:09.fits
        snprintf(path, 100, "%s%s", dir[id], file_name);
//        log.debug("[DISK ] Thread %d: Going to write %s.", id, path);
        if (is_exist(path)) {   // 文件已存在
            // 索引中不一定存在，还是要将 file 从 wt_list 移动到 file_list
            Key key(file->ra, file->dec, file->time);
            disk->file_list->insert(PAIR(key, *file));
            disk->wt_file_list->erase(iter);
//            log.info("[DISK ] Thread %d: Skip writing %s to disk %d for already existence.", id, file_name, id);
        } else {    // 文件不存在
            gen_file(tmp_dir, file_name);
            if (!system_call("cp %s%s %s", tmp_dir, file_name, dir[id])) { // 写入成功
                // 将 file 从 wt_list 移动到 file_list
                Key key(file->ra, file->dec, file->time);
                disk->file_list->insert(PAIR(key, *file));
                disk->wt_file_list->erase(iter);
                log.info("[DISK ] Thread %d: Have wrote %s to disk %d.", id, file_name, id);
            } else {
                log.error("[DISK ] Thread %d: Fail to write file %s.", id, file_name);
            }
            // 删除拷贝到内存中的文件
            if (system_call("rm -f %s%s", tmp_dir, file_name)) {
                log.error("[DISK ] Thread %d: Fail to delete file in memory.", id);
            }
        }
        iter = disk->wt_file_list->begin();
    }
}

int read_file(const int id, const char *path) {
    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        log.error("[DISK ] Thread %d: Failed to open file %s.", id, path);
        return -1;
    }
    log.debug("[DISK ] Thread %d   : Open file %s.", id, path);
    int file_size = config.get_int("DATA", "FileSize", 200);
    char buffer[1025];  // 1024 also OK
    for (int i = 0; i < file_size - 1; ++i) {
        for (int j = 0; j < 1024; ++j) {
            fread(buffer, 1024, 1, fp);
        }
    }
    log.debug("[DISK ] Thread %d   : Close file.", id);
    fclose(fp);
//    File file(path, "rb");
//    if (file.is_null()) {
//        return -1;
//    }
//    int file_size = config.get_int("DATA", "FileSize", 200);
//    char buffer[1025];  // 1024 also OK
//    for (int i = 0; i < file_size - 1; ++i) {
//        for (int j = 0; j < 1024; ++j) {
//            file.read(buffer, 1024, 1);
//        }
//    }
    return 0;
}

// 将指定磁盘的读队列处理掉
// @parm id 磁盘ID
void handle_rd(int id) {
    DiskInfo *disk = data_disk_array[id];
    RW_LIST* rd_list = disk->rd_file_list;
    log.debug("[DISK ] Thread %d: Going to handle %d file reads.", id, rd_list->size());

//    log.sublog("--------------------------- disk %d -------- rd_file_list --------------------\n", id);
//    RW_LIST::iterator rd_iter;
//    for (rd_iter = rd_list->begin(); rd_iter != rd_list->end(); ++rd_iter) {
//        show_file(&rd_iter->second);
//    }
//    log.pure("\n");

    RW_LIST::iterator iter = rd_list->begin();
    int count = 0;
    while (iter != rd_list->end()) {
        // 传输数据
        FileInfo *file = &iter->second;
        char file_name[100] = {0};
        int ret = get_file_name(file, file_name, sizeof(file_name));
        char path[100] = {0}; // 足够大，例如 /media/hdd00/fits/332.4060_-56.6292_2016-03-14_17:41:09.fits
        snprintf(path, 100, "%s%s", dir[id], file_name);
        if (ret || !is_exist(path)) {    // 解析文件名失败 或 文件不存在
            log.error("[DISK ] Thread %d[%d]: Failed to get file name: %s.", id, count, file_name);
            // 将 file 从 rd_list 直接删除
//            hand_over_a_file(iter->second.file_id);
//            log.error("[DISK ] Thread %d[%d]: After hand_over_a_file()", id, count);
            show_file(file);
            rd_list->pop_front();
            log.error("[DISK ] Thread %d[%d]: After pop_front()", id, count);
            iter = rd_list->begin();
//            log.error("[DISK ] Thread %d[%d]: After update iter", id, count);
            ++count;
            rd_list->clear();
            break;
        }
//        log.debug("[DISK ] Thread %d[%d]: Going to read file %s.", id, count, file_name);
//        char t_dir[100] = {0};
//        snprintf(t_dir, 100, "/media/hdd0%d/", id);
//        char t_file[100] = "haha.fits";
//        gen_file(t_dir, t_file);
        if (!sleep(1)) { // 读取成功 !read_file(id, path)  !sleep(1)
            log.debug("[DISK ] Thread %d[%d]: Read file success!", id, count);
        } else {
            log.error("[DISK ] Thread %d[%d]: Fail to read file %s.", id, count, file_name);
        }
        hand_over_a_file(file->file_id);
        iter = rd_list->erase(iter);    // 将 file 从 rd_list 直接删除
        // 删除拷贝到内存中的文件
//        if (system_call("rm -f %s%s", tmp_dir, file_name)) {
//            log.error("[DISK ] Thread %d: Fail to delete file in memory.", id);
//        }
//        iter = rd_list->begin();
        ++count;
    }
    log.debug("[DISK ] Thread %d: Totally read %d files!", id, count);
}

/*
 * 每个磁盘一个线程，用来处理读写队列的任务
 */
void *io_handler_t(void *_arg) {
    sleep(1);
    Args *arg = (Args *)_arg;
    int id = arg->id;
    if (id > 7) {   // 实际只装载了8个磁盘
        return NULL;
    }

    if (config.get_int("MAIN", "InitDisk", 1)) {
        system_call("hdparm -B 50 %s", dev[id]);   // 允许磁盘 spin-down
        int mode = config.get_int("MAIN", "Mode", 0);
        int idle_th = config.get_int("MAIN", "MaxIdleTime", 60);
        if (mode == 0) {    // 普通模式
            system_call("hdparm -S %d %s", idle_th/5, dev[id]);
        } else {            // DAES
            system_call("hdparm -S %d %s", idle_th*2/5, dev[id]);
        }
    }

    int max_req_time = config.get_int("REQ", "MaxReqTime", 1000);
    DiskInfo *disk = data_disk_array[id];
    while (exp_time <= max_req_time) {
        if (disk->disk_state < 0) {
            usleep(200000); //休眠 0.2 秒
            continue;
        }
        int rd_size = disk->rd_file_list->size();
        int wt_size = disk->wt_file_list->size();
        // 有任务时处理任务
        if (rd_size || wt_size) {
//            log.debug("[DISK ] Thread %d: %d files to write, %d files to read.", id, wt_size, rd_size);
            disk->disk_state = 0;
            if (wt_size) {
                handle_wt(id);
            }
            if (rd_size) {
                handle_rd(id);
            }
        }
        usleep(200000); //休眠 0.2 秒
    }

    return NULL;
}

Disk_Ctl::Disk_Ctl() {
    _disk_num = data_disk_num;
//    _disk_state = new int[_disk_num]();
    _io_handler = new pthread_t[_disk_num];
    // 初始化所有磁盘的设备名、存放目录
    dev = new char*[_disk_num];
    dir = new char*[_disk_num];
    for (int i = 0; i < _disk_num; ++i) {
        dev[i] = new char[50]();
        sprintf(dev[i], "/dev/sd%c", 'b'+i);    // 设备名，例如 /dev/sdb
        dir[i] = new char[50]();
        sprintf(dir[i], "/media/hdd0%c/fits/", '0'+i);  // 目录名，例如 /media/hdd00/fits/
    }

    argss = new Args*[_disk_num];
    for (int i = 0; i < _disk_num; ++i) {
        argss[i] = new Args;
        argss[i]->id = i;
        pthread_create(&_io_handler[i], NULL, io_handler_t, (void *)argss[i]);
    }
    log.debug("[DISK ] Create %d threads.", _disk_num);
}

Disk_Ctl::~Disk_Ctl() {
    for (int i = 0; i < _disk_num; ++i) {
        pthread_join(_io_handler[i], NULL);
    }
    delete _io_handler;
    _io_handler = NULL;
    for (int i = 0; i < _disk_num; ++i) {
        delete argss[i];
        delete dir[i];
        delete dev[i];
    }
    delete argss;
    argss = NULL;
    delete dir;
    dir = NULL;
    delete dev;
    dev = NULL;

    _disk_num = -1;
}

// 成功返回 0，失败返回 1
int Disk_Ctl::spin_down_disk(int id) {
    if (system_call("hdparm -y %s", dev[id])) {
        log.error("[DISK ] Fail to spin down disk %d.", id);
        return 1;
    }
    return 0;
}
