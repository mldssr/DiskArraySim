/*
 * disk_ctl.cpp
 *
 *  Created on: Nov 17, 2018
 *      Author: lxx
 */
#include "pthread.h"
#include "unistd.h"

#include "utils/basic.h"
#include "utils/file.h"
#include "utils/config.h"
#include "utils/log.h"
#include "model.h"
#include "data.h"
#include "disk_ctl.h"

// 用于处理实际 I/O

typedef struct Args {
    int id;  // 磁盘编号
};

Args **argss = NULL;
char **dev = NULL;                  // 设备名，例如 /dev/sdb
char **dir = NULL;                  // 目录名，例如 /media/hdd00/fits/
const char *tmp_dir = "/dev/shm/";  // 临时内存目录，读写缓冲区

// 处理写队列
// @parm id 磁盘ID
void handle_wt(int id) {
    log.debug("[DISK ] Thread %d: handle_wt() begin.", id);
    DiskInfo *disk = data_disk_array[id];
    RW_LIST::iterator iter = disk->wt_file_list->begin();
    while (iter != disk->wt_file_list->end()) {
        // 传输数据
        FileInfo *file = &iter->second;
        char *file_name = get_file_name(file);
        char path[100]; // 足够大，例如 /media/hdd00/fits/332.4060_-56.6292_2016-03-14_17:41:09.fits
        sprintf(path, "%s%s", dir[id], file_name);
//        log.debug("[DISK ] Thread %d: Going to write %s.", id, path);
        if (is_exist(path)) {   // 文件已存在
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
        delete file_name;
        iter = disk->wt_file_list->begin();
    }
}

// 处理读队列
// @parm id 磁盘ID
void handle_rd(int id) {
    DiskInfo *disk = data_disk_array[id];
    RW_LIST::iterator iter = disk->rd_file_list->begin();
    while (iter != disk->rd_file_list->end()) {
        // 传输数据
        FileInfo *file = &iter->second;
        char *file_name = get_file_name(file);
        if (!system_call("cp %s%s %s", dir[id], file_name, tmp_dir)) { // 读取成功
            // 将 file 从 rd_list 直接删除
            hand_over_a_file(iter->second.file_id);
            disk->rd_file_list->erase(iter);
        } else {
            log.error("[DISK ] Thread %d: Fail to read file %s.", id, file_name);
        }
        // 删除拷贝到内存中的文件
        if (system_call("rm -f %s%s", tmp_dir, file_name)) {
            log.error("[DISK ] Thread %d: Fail to delete file in memory.", id);
        }
        delete file_name;
        iter = disk->rd_file_list->begin();
    }
}

/*
 * 每个磁盘一个线程，用来处理读写队列的任务
 */
void *io_handler_t(void *_arg) {
    sleep(2); // 休眠 2 秒
    Args *arg = (Args *)_arg;
    int id = arg->id;
    if (id > 7) {   // 实际只装载了8个磁盘
        return NULL;
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
            log.debug("[DISK ] Thread %d: %d files to write, %d files to read.", id, wt_size, rd_size);
            disk->disk_state = 0;
            handle_wt(id);
            handle_rd(id);
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
        dev[i] = new char[20];
        sprintf(dev[i], "/dev/sd%c", 'b'+i);    // 设备名，例如 /dev/sdb
        dir[i] = new char[20];
        sprintf(dir[i], "/media/hdd0%c/fits/", '0'+i);  // 目录名，例如 /media/hdd00/fits/
    }

    system_call("hdparm -B 50 /dev/sd[b-i]");   // 允许磁盘 spin-down
    int mode = config.get_int("MAIN", "Mode", 0);
    int idle_th = config.get_int("MAIN", "MaxIdleTime", 60);
    if (mode == 0) {    // 普通模式
        system_call("hdparm -S %d /dev/sd[b-i]", idle_th/5);
    } else {            // DAES
        system_call("hdparm -S %d /dev/sd[b-i]", idle_th*2/5);
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
