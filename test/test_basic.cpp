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
#include <assert.h>

#include "utils/basic.h"

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

int main(int argc, char **argv) {
    test_stradd();
    test_strbase();
    test_strext();
    test_strcount();
    test_strtrim();
    test_strfit();
    test_strreplace();

    return 0;
}
