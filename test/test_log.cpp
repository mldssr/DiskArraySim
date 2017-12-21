/*
 * test_log.cpp
 *
 *  Created on: Oct 23, 2016
 *      Author: sunchao
 */
/*  测试log模块，记录debug, info, error型日志。
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "utils/log.h"

static int test_log()
{
    int num = 2;
    char str[] = "III";

    log.debug("Frist entry of the log!");
    log.info("The No. %d entry!", num++);
    log.error("The %s entry!", str);

    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Need a parameter of log path!");
        exit(1);
    }
    char *logpath = argv[1];

    assert(log.started() == false);

    printf("Initialize Log.\n");
    log.init(logpath);
    assert(log.started() == true);

    printf("Test Log.\n");
    /* test the routine log writing. */
    assert(test_log() == 0);

    printf("All tests passed!\n");
    return 0;
}



