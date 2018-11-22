/*
 * test_basic.cpp
 *
 *  Created on: Jan 15, 2017
 *      Author: sunchao
 */
/* 测试基础函数：stradd, strbase, strext, strcount, strtrim, strfit, strreplace
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include "utils/basic.h"

static void test_MAX() {
    double a = 1.3, b = 2.6;
    double c = MAX(a, b);
    assert(c == b);
    printf("MAX of %f and %f is %f\n", a, b, c);
    printf("Test MAX passed!\n");
}

//static void test_system_call() {
//    printf("Copy begin.\n");
//    int ret = system_call("cp /home/lxx/Media/ready.player.one.2018.720p.web.h264-webtiful.mkv /home/lxx/Media/real.mkv");
//    if (ret == 0) {
//        printf("Copy successfully.\n");
//    } else {
//        printf("Copy failed.\n");
//    }
//}

static void test_system_callback() {
    char output[1024];
    if (!system_callback(output, sizeof(output), "ls -l /dev %s", " | grep sd")) {
        printf("Error occurs when system_call()!\n");
    }
    printf("%s\n", output);
    if (!system_callback(output, sizeof(output), "ipmitool sdr get PSU1_POUT | awk 'NR==4 {print $3}'")) {
        printf("Error occurs when system_call()!\n");
    }
    printf("%s\n", output);
    if (!system_callback(output, sizeof(output), "NoCmd")) {
        printf("Error occurs when system_call()!\n");
    }
    printf("%s\n", output);
}

static void test_stradd() {
    char *get;
    get = stradd("Hello", " World!");
    assert(strcmp(get, "Hello World!") == 0);
    delete get;

    get = stradd("Hello", " ", "World!");
    assert(strcmp(get, "Hello World!") == 0);
    delete get;

    printf("Test stradd passed!\n");
}

static void test_strbase() {
    char *get;
    get = strbase("/home/xxx/test", '/');
    assert(strcmp(get, "/home/xxx") == 0);
    delete get;

    get = strbase("test", '/');
    assert(strcmp(get, "") == 0);
    delete get;

    printf("Test strbase passed!\n");
}

static void test_strext() {
    char *get;
    get = strext("/home/xxx/test", '/');
    assert(strcmp(get, "test") == 0);
    delete get;

    get = strext("test", '/');
    assert(strcmp(get, "test") == 0);
    delete get;

    printf("Test strext passed!\n");
}

static void test_strcount() {
    int cnt = strcount("/home/xxx/test", '/');
    assert(cnt == 3);
    cnt = strcount("test", '/');
    assert(cnt == 0);

    printf("Test strcount passed!\n");
}

static void test_strtrim() {
    char *get;
    get = strtrim("   ");
    assert(strcmp(get, "") == 0);
    delete get;

    get = strtrim(" test  ");
    assert(strcmp(get, "test") == 0);
    delete get;

    printf("Test strtrim passed!\n");
}

static void test_strfit() {
    int fit = strfit("tar.gz", "*.gz");
    assert(fit == 0);

    fit = strfit("tar.gza", "*.gz");
    assert(fit != 0);

    fit = strfit("tar.gza", "*.gz*");
    assert(fit == 0);

    printf("Test strfit passed!\n");
}

static void test_strreplace() {
    char *get;
    get = strreplace("Hello", "H", "h");
    assert(strcmp(get, "hello") == 0);
    delete get;

    get = strreplace("Hello", "l", "[ ]");
    assert(strcmp(get, "He[ ][ ]o") == 0);
    delete get;

    printf("Test strreplace passed!\n");
}

static void test_str2hex() {
    const char *str = "0123";
    char *get = str2hex(str, 1);
    assert(strcmp(get, "30") == 0);
    delete get;

    get = str2hex(str, 4);
    assert(strcmp(get, "30313233") == 0);
    delete get;

    printf("Test str2hex passed!\n");
}

static void test_time() {
    char buf[20];
    strcpy(buf, "2017-12-20T23:33:59");
    buf[10] = ' ';
    time_t time = str2time_t(buf);
//    printf("Convert str to time_t: %ld\n", time);

    char time_buf[20];
    time_t2str(time, time_buf, 20);
    assert(strcmp(buf, time_buf) == 0);
    for (int i = 1; i <= 20; i++) {
        memset(time_buf, 0, sizeof(time_buf));
        time_t2str(time, time_buf, i);
        printf("Convert time_t to str with buf size %d: %s\n", i, time_buf);
    }

    time_t t_0 = str2time_t("2016-03-14 08:00:00");
    time_t t_1 = str2time_t("2016-03-14 23:59:59");
    time_t t_2 = str2time_t("2016-03-15 00:00:00");
    time_t t_3 = str2time_t("2016-03-15 07:59:59");
    int days_0 = t_0 / 3600 / 24;
    int days_1 = t_1 / 3600 / 24;
    int days_2 = t_2 / 3600 / 24;
    int days_3 = t_3 / 3600 / 24;
    assert(days_0 == days_1 && days_1 == days_2 && days_2 == days_3);
    printf("Test time passed!\n");
}

static void test_random() {
    for (int i = 0; i < 10; i++) {
        printf("%d\n", get_random(10, 20));
    }
    printf("Test random passed!\n");
}

int main(int argc, char **argv) {
    test_system_callback();
//    test_MAX();
//    test_stradd();
//    test_strbase();
//    test_strext();
//    test_strcount();
//    test_strtrim();
//    test_strfit();
//    test_strreplace();
//    test_str2hex();
//    test_time();
//    test_random();
    return 0;
}
