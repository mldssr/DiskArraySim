/*
 * config.cpp
 *
 *  Created on: 4 8, 2017
 *      Author: sunchao
 */
/*  测试文件读写功能。
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>

#include "utils/file.h"

int test_file(char *filename) {
    File fp1(filename, "w");
    fp1.print("Hello");
    fp1.close();

    char message[128];
    File fp2(filename, "r");
    fp2.scan("%s", message);
    fp2.close();
    printf("%s\n", message);

    File fp3(filename, "r+");
    fp3.seek(3);
    fp3.print("world!");
    fp3.close();

    File fp4(filename, "r");
    fp4.scan("%s", message);
    printf("%s\n", message);
    fp4.close();
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Need a parameter of filename!");
        exit(1);
    }
    char *filename = argv[1];

    test_file(filename);
    printf("All tests passed!\n");

    return 0;
}


