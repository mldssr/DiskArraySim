/*
 * config.cpp
 *
 *  Created on: Oct 22, 2016
 *      Author: sunchao
 */
/*  测试config的基础功能：
 *  1. 初始化配置文件
 *  2. 以字符串输出所有配置
 *  3. 读取int型配置
 *  4. 读取double型配置
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "utils/config.h"

#define LEN        256

//Integers:
char keys[5][LEN] = {
    "MaxTaskNum",
    "Port",
    "TimePerCheck",
    "MaxRedoNum",
    "Threshold",
};

static int test_read() {
    config.print_all();
    return 0;
}

static int test_int() {
    for (int i = 0; i < 5; i++) {
        int value = config.get_int("Runtime", keys[i], 0);
        printf("%s = %d\n", keys[i], value);
    }
    return 0;
}

static int test_double() {
    double value = config.get_double("Runtime", "Timeout", 0.0);
    printf("%s = %lf\n", "Timeout", value);
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Need a parameter of filename!");
        exit(1);
    }
    char *filename = argv[1];

    config.init(filename);
    assert(!test_read());
    assert(!test_int());
    assert(!test_double());
    printf("All tests passed!\n");

    return 0;
}
