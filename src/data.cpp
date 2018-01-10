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

