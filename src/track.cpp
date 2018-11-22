/*
 * track.cpp
 *
 *  Created on: Nov 17, 2018
 *      Author: lxx
 */

#include "utils/file.h"
#include "utils/config.h"
#include "utils/log.h"
#include "model.h"
#include "corr.h"

/*
 * WD10EZEX
 * Power Management
 * 12VDC ±10% (A, peak)             2.5
 * Average power requirements (W)
 * Read/Write                       6.8
 * Idle                             6.1
 * Standby/Sleep                    1.2
 */
double ideal_power(int state) {
    double power = 0.0;
    if (state == 0 - disk_start_time) {     // 关闭
        power = 1.2;
    } else if (state < 0) {                 // 启动中
        power = 30.0;
    } else if (state == 0) {                // Read/Write
        power = 6.8;
    } else {                                // Idle
        power = 6.1;
    }
    return power;
}

double ideal_total_power() {
    double total_power = 0.0;
    for (int i = 0; i < data_disk_num; ++i) {
        total_power += ideal_power(data_disk_array[i]->disk_state);
    }
    return total_power;
}

double real_total_power() {
    double power = 0;
    char buffer[100];
    if (!system_callback(buffer, sizeof(buffer), "ipmitool sdr get PSU1_POUT | awk 'NR==4 {print $3}'")) {
        log.error("[TRACK] Fail to call ipmitool.");
    }
    sscanf(buffer, "%lf", &power);
    return power;
}

// 构造小顶堆
#define leftChild(i) (2*(i) + 1)

static void percDown(double *arr, int i, int N) {
    int child;
    double tmp;
    for (tmp = arr[i]; leftChild(i) < N; i = child) {
        child = leftChild(i);
        // 左右子节点中选取较小的一个
        if (child != N - 1 && arr[child + 1] < arr[child]) {
            child++;
        }
        if (arr[child] < tmp)
            arr[i] = arr[child];
        else
            break;
    }
    arr[i] = tmp;
}

//void HeapSort(double *arr, int N) {
//    int i;
//    // 构造小顶堆
//    for (i = N / 2; i >= 0; i--)
//        percDown(arr, i, N);
//    // 将堆顶和堆底元素交换，之后将底部上升，最后重新调用Min-Heapify保持最大堆性质
//    for (i = N - 1; i > 0; i--) {
//        swap1(&arr[0], &arr[i]);
//        percDown(arr, 0, i);
//    }
//}

double ideal_peak_power[100];   // 存放 Top 100 理想功率
double real_peak_power[100];    // 存放 Top 100 真实功率
static int total_opened = 0;    // SUM(每秒运行中磁盘数)
static File *shot_file = NULL;       // 快照文件：记录每秒钟所有磁盘的状态
static File *power_shot_file = NULL; // 记录每秒的磁盘总功耗

// 将 power 插入到堆 peak_power 中
static void update_peak_power(double *peak_power, double power) {
    if (power > peak_power[0]) {
        peak_power[0] = power;
        percDown(peak_power, 0, 100);
    }
}

// 返回 peak_power 的平均数
static double get_avg_peak_power(double *peak_power) {
    double power = 0.0;
    for (int i = 0; i < 100; ++i) {
        power += peak_power[i];
    }
    power /= 100;
    return power;
}

/*
 * 创建记录文件 snapshot.csv，写入标题行
 */
static void power_shot_init() {
    char *snap_shot_file = config.get_string("TRACK", "PowershotFile", "./track/power_shot.csv");
    power_shot_file = new File(snap_shot_file, "w");
    // 写入第一行
    power_shot_file->print("time");
    power_shot_file->print(",total_power");
    power_shot_file->print("\n");
}

/*
 * 快照内容：
 * time   total_power
 * 时间    磁盘组总功率
 */
void power_shot() {
    if (power_shot_file == NULL) {
        power_shot_init();
    }
    double power = real_total_power();
    power_shot_file->print("%5d", exp_time);   // 写入时间
    power_shot_file->print(",%.1f", power);   // 写入总功率
    power_shot_file->print("\n");

    update_peak_power(real_peak_power, power);
}

/*
 * 创建记录文件 snapshot.csv，写入标题行
 */
static void snapshot_init() {
    char *snap_shot_file = config.get_string("TRACK", "SnapshotFile", "./track/snapshot.csv");
    shot_file = new File(snap_shot_file, "w");
    // 写入第一行
    shot_file->print("time");
    for (int i = 0; i < data_disk_num; i++) {
        // disk_0, disk_1, disk_2, ...
        shot_file->print(",disk_%d", i);
    }
    shot_file->print(",opened");
    shot_file->print(", ");      // 留一列空白
    for (int i = 0; i < data_disk_num; i++) {
        // tasks_0, tasks_1, tasks_2, ...
        shot_file->print(",tasks_%d", i);
    }
    if (config.get_int("MAIN", "Mode", 0) == 0) {       // 普通模式下不需要记录命中指数部分
        shot_file->print("\n");
        return;
    }
    shot_file->print(", ");      // 留一列空白
    for (int i = 0; i < data_disk_num; i++) {
        // prob_0, prob_1, prob_2, ...
        shot_file->print(",prob_%d", i);
    }
    shot_file->print("\n");
}

/*
 * 快照内容：
 * time   disk_i       opened        tasks_i                prob_i
 * 时间    磁盘i的状态    运行中磁盘数    磁盘i的正在读写的文件数    磁盘i的命中指数
 */
void snapshot() {
    if (shot_file == NULL) {
        snapshot_init();
    }
    // 写入时间
    shot_file->print("%5d", exp_time);
    // 写入当前各个磁盘状态
    int opened = 0;
    for (int i = 0; i < data_disk_num; i++) {
        shot_file->print(",%2d", data_disk_array[i]->disk_state);
        if (data_disk_array[i]->disk_state >= 0) {
            opened++;
        }
    }
    shot_file->print(",%2d", opened);
    total_opened += opened;
    // 写入当前各个磁盘任务数
    shot_file->print(", ");
    for (int i = 0; i < data_disk_num; i++) {
        int tasks = data_disk_array[i]->rd_file_list->size() + data_disk_array[i]->wt_file_list->size();
        shot_file->print(",%d", tasks);
    }
    if (config.get_int("MAIN", "Mode", 0) == 0) {       // 普通模式下不需要记录命中指数部分
        shot_file->print("\n");
        return;
    }
    // 写入各个磁盘命中概率
    shot_file->print(", ");
    for (int i = 0; i < data_disk_num; i++) {
        shot_file->print(",%8.2f", data_disk_hit_prob[i]);
    }
    shot_file->print("\n");

    update_peak_power(ideal_peak_power, ideal_total_power());

    power_shot();
}

/*
 * 实验结束时，再统计一下各个磁盘的运行情况，记录到 disk_stat.txt
 */
void snapshot_end() {
    delete shot_file;
    shot_file = NULL;
    delete power_shot_file;
    power_shot_file = NULL;

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
    disk_stat.print("\n");
    disk_stat.print("Average top 100 peak power: %f W.\n", get_avg_peak_power(real_peak_power));
}
