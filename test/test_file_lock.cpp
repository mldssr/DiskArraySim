/*
 * config.cpp
 *
 *  Created on: 4 8, 2017
 *      Author: sunchao
 */
/*  测试文件加锁解锁功能，包括加锁过程中删除文件依然可用。
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>

#include "utils/file.h"

int test_lock(char *filename) {
    File fp(filename, "w");
    while (1) {
        int ret = fp.lock();
        if (ret < 0) {
            printf("Lock file returned: %d: %s.\n", ret, strerror(errno));
        } else {
            printf("Lock successful!\n");
            break;
        }
        sleep(1);
    }
    return 0;
}

int test_unlock(char *filename) {
    File fp(filename, "w");
    while (1) {
        int ret = fp.unlock();
        if (ret < 0) {
            printf("Unlock file returned: %d: %s.\n", ret, strerror(errno));
        } else {
            printf("Unlock successful!\n");
            break;
        }
        sleep(1);
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Need a parameter of filename!");
        exit(1);
    }
    char *filename = argv[1];

    test_lock(filename);
    sleep(1);
    test_unlock(filename);
    test_lock(filename);
    delete_file(filename);
    sleep(1);
    test_unlock(filename);
    printf("All tests passed!\n");

    return 0;
}


