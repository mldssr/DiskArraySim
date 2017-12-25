/*
 * req.h
 *
 *  Created on: Dec 23, 2017
 *      Author: lxx
 */

#ifndef SRC_REQ_H_
#define SRC_REQ_H_

struct Req {
    int time;                   // 请求生成时间
    double ra;
    double dec;
    char time_str[11];          // "2017-03-11"
};

#define R_MAP     std::multimap<int, Req>
#define R_PAIR    std::pair<int, Req>

extern R_MAP req_list;

/*
 * 读取[REQ]部分参数，生成请求序列，导入到csv中
 */
void gen_req();

/*
 * 读取[REQ]部分参数，从csv文件中提取req，到req_list中
 */
void get_req();

#endif /* SRC_REQ_H_ */
