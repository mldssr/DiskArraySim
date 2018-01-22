/*
 * data.h
 *
 *  Created on: Dec 12, 2017
 *      Author: lxx
 */

#ifndef SRC_DATA_H_
#define SRC_DATA_H_

#include "model.h"

/*
 * 解析文件file_name中的所有条目，依次存入到 data_disk_array 中
 * @parm file_name 全路径
 * @return 0-success, 1-failed
 */
int parse_file(const char *file_name);

/*
 * 解析文件夹dir中的所有文件，依次存入到 data_disk_array 中
 * @return 0-success
 */
int scan_data(const char *dir);

/*
 * 工具：分析 data_disk_array 中的数据，将结果写入到 footprint.txt
 */
void footprint();

#endif /* SRC_DATA_H_ */
