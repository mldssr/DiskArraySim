/*
 * data.cpp
 *
 *  Created on: Dec 12, 2017
 *      Author: lxx
 */
#include "time.h"
#include "string.h"

#include "utils/basic.h"
#include "utils/file.h"
#include "utils/log.h"
#include "utils/config.h"
#include "data.h"

/*
 * 解析文件file_name中的所有条目，依次存入到 data_disk_array 中
 * @parm file_name 全路径
 * @return 0-success, 1-failed
 */
int parse_file(const char *file_name) {
    File file(file_name, "r");
    char line[200];
    char buf[6][100];
    double ra, dec;
    time_t time;
    struct tm* tmp_time = new tm;
    // 读取表头
    // FILENAME       TYPE            RA       DEC      exptime DATE
    file.readline(line, 200);

    while (!file.is_eof()) {
        // 读取一行记录
        // b160406.000001.fits             FLAT  226.5075 -72.6075    3.00 2016-04-07T01:37:04
        memset(line, 0, sizeof(line));
        file.readline(line, 200);
        // 处理完最后一行后还会进入一次循环，读取到空字符串，排除之
        if (strlen(line) == 0) {
            continue;
        }
//        log.debug("[DATA] Read length: %d", strlen(line));

        int ret = sscanf(line, "%s %s %s %s %s %s\n", buf[0], buf[1], buf[2],
                buf[3], buf[4], buf[5]);
        if (ret != 6) {
//            log.info("[DATA] Ignore illegal record: %s", line);
            continue;
        }
//        log.debug("[%d] %s %s %s %s %s %s", ret, buf[0], buf[1], buf[2], buf[3],
//                buf[4], buf[5]);
        if (sscanf(buf[2], "%lf", &ra) == 0
                || sscanf(buf[3], "%lf", &dec) == 0
                || strptime(buf[5], "%Y-%m-%dT%H:%M:%S", tmp_time) == NULL) {
            log.error("[DATA] Fail to strptime");
            delete tmp_time;
            return 1;
        }

        // 按当地时区解析tmp_time
        time = mktime(tmp_time);

        // 排除逻辑上错误的数据，对于 (AST3 2016)，等价于 exptime == 0.0
        if (ra < 0.0 || ra > 360.0 || dec < -90.0 || dec > 90.0) {
//            log.sublog("[DATA] Invalid data from %s.\n", file_name);
            continue;
        }

        FileInfo *file = new_FileInfo(file_id_num++,
                config.get_int("DATA", "FileSize", 200), ra, dec, time);
        add_file(file);
//        show_file(file);
        delete file;
    }
    delete tmp_time;
    return 0;
}

/*
 * 解析文件夹dir中的所有文件，依次存入到 data_disk_array 中
 * @return 0-success, 1-failed
 */
int scan_data(const char *dir) {
    const char *file_list = "files_list.txt";
    // 获取dir文件夹中所有文件名，存到 files_list.txt 中
    if (system_call("ls -al %s | awk 'NR>3 {print $9}' > %s", dir, file_list) != 0) {
        log.error("[DATA] Fail to get file list of DIR [%s].", dir);
        return 1;
    }

    File file(file_list, "r");
    char buf[100];
    char *file_name = NULL;
    while (!file.is_eof()) {
        memset(buf, 0, sizeof(buf));
        file.readline(buf, 100);
        // 处理完最后一行后还会进入一次循环，读取到空字符串，排除之
        if (strlen(buf) == 0) {
            continue;
        }
        char *buf_trim = strtrim(buf);
        file_name = stradd(dir, "/", buf_trim);
        delete buf_trim;
        log.info("[DATA] Going to parse file %s", file_name);

        if (parse_file(file_name)) {
            log.error("[DATA] Fail to parse FILE %s", file_name);
            return 1;
        }

        delete file_name;
        file_name = NULL;
    }
    return 0;
}


/*
 * 工具：分析 data_disk_array 中的数据，将结果写入到 footprint.txt
 */
void footprint() {
    File file("footprint.txt", "w");
    file.print("  disk,    ra_min,    ra_max,   dec_min,   dec_max\n");

    double ra_min = 360.0;
    double ra_max = 0.0;
    double dec_min = 90.0;
    double dec_max = -90.0;

    for (int i = 0; i < data_disk_num; i++) {
        // 用来统计这个磁盘上的范围
        double sub_ra_min = 360.0;
        double sub_ra_max = 0.0;
        double sub_dec_min = 90.0;
        double sub_dec_max = -90.0;

        // 只考虑 file_list
        MAP* file_list = data_disk_array[i]->file_list;
        MAP::iterator iter;
        for (iter = file_list->begin(); iter != file_list->end(); iter++) {
            double ra = iter->second.ra;
            double dec = iter->second.dec;
            // 更新这个磁盘的范围
            if (ra < sub_ra_min) {
                sub_ra_min = ra;
            }
            if (ra > sub_ra_max) {
                sub_ra_max = ra;
            }
            if (dec < sub_dec_min) {
                sub_dec_min = dec;
            }
            if (dec > sub_dec_max) {
                sub_dec_max = dec;
            }
        }
        file.print("disk_%d, %9.4f, %9.4f, %9.4f, %9.4f\n", i, sub_ra_min, sub_ra_max, sub_dec_min, sub_dec_max);

        // 更新全局范围
        if (sub_ra_min < ra_min) {
            ra_min = sub_ra_min;
        }
        if (sub_ra_max > ra_max) {
            ra_max = sub_ra_max;
        }
        if (sub_dec_min < dec_min) {
            dec_min = sub_dec_min;
        }
        if (sub_dec_max > dec_max) {
            dec_max = sub_dec_max;
        }
    }

    file.print("\n");
    file.print(" whole, %9.4f, %9.4f, %9.4f, %9.4f\n", ra_min, ra_max, dec_min, dec_max);
}
