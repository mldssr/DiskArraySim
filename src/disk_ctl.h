/*
 * disk_ctl.h
 *
 *  Created on: Nov 17, 2018
 *      Author: lxx
 */
#ifndef SRC_DISK_CTL_H_
#define SRC_DISK_CTL_H_
#include "pthread.h"

class Disk_Ctl {
private:
    int _disk_num;           // 磁盘数
    pthread_t *_io_handler;  // 线程数组
public:
    Disk_Ctl();
    ~Disk_Ctl();
//    int *_disk_state;           // 磁盘状态
    static int spin_down_disk(int i);  // 降低磁盘 i 转速
//    int check_state();          // 命令行查询状态，并更新
};

#endif /* SRC_DISK_CTL_H_ */
