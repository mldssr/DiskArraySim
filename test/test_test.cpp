/*
 * test_test.cpp
 *
 *  Created on: Dec 11, 2017
 *      Author: lxx
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <string>
#include <list>

#include "utils/log.h"
#include "utils/config.h"
#include "utils/file.h"
#include "model.h"

using namespace std;

static void test_read_file() {
    struct dirent *ptr;
    DIR *dir;
    dir = opendir("./data");
    printf("文件列表:\n");
    while ((ptr = readdir(dir)) != NULL) {
        //跳过'.'和'..'两个目录
        if (ptr->d_name[0] == '.')
            continue;
        printf("%s\n", ptr->d_name);
    }
    closedir(dir);
}

static int test_up_lower_bound() {
    std::map<char, int> mymap;
    std::map<char, int>::iterator itlow, itup;

//   mymap['a']=20;
    mymap['b'] = 40;
    mymap['c'] = 60;
    mymap['d'] = 80;
    mymap['e'] = 100;
    mymap['h'] = 120;
    mymap['l'] = 140;

    // print content:
    for (std::map<char, int>::iterator it = mymap.begin(); it != mymap.end();
            ++it)
        std::cout << it->first << " => " << it->second << '\n';

    /*
     * NOTE:
     * lower_bound 返回 >= 给定key的第一个值，若没有則返回 end()
     * upper_bound 返回 >  给定key的第一个值，若没有則返回 end()
     */
    printf("\na\n");
    itlow = mymap.lower_bound('a');  // itlow points to b
    std::cout << itlow->first << " => " << itlow->second << '\n';
    itup = mymap.upper_bound('a');   // itup points to b
    std::cout << itup->first << " => " << itup->second << "\n\n";

    printf("d\n");
    itlow = mymap.lower_bound('d');  // itlow points to d
    std::cout << itlow->first << " => " << itlow->second << '\n';
    itup = mymap.upper_bound('d');   // itup points to e (not d!)
    std::cout << itup->first << " => " << itup->second << "\n\n";

    printf("f\n");
    itlow = mymap.lower_bound('f');  // itlow points to h
    std::cout << itlow->first << " => " << itlow->second << '\n';
    itup = mymap.upper_bound('f');   // itup points to h
    std::cout << itup->first << " => " << itup->second << "\n\n";

    printf("p\n");
    itlow = mymap.lower_bound('p');  // itlow points to end()
    std::cout << itlow->first << " => " << itlow->second << '\n';
    itup = mymap.upper_bound('p');   // itup points to end()
    std::cout << itup->first << " => " << itup->second << "\n\n";
    printf("itlow == mymap.end()? %s\n", itlow == mymap.end()? "true" : "false");  // true
    printf("itup == mymap.end()? %s\n", itup == mymap.end()? "true" : "false");  // true

//    mymap.erase(itlow, itup);        // erases [itlow,itup)

    return 0;
}

static void test_read_data() {
//    File file("./AST3-2.obs.log.20160000.txt", "r");
    char buf[6][200];
    buf[5][0] = 0;
//    file.readline(buf[0], 200);
    const char *line = "b160406.000002.fits   , ,  type        226.5075 -72.6075    3.00 2016-04-07T01:37:26    ";
    for (int i = 0; i < 1; i++) {
        double ra, dec, exp;
        int ret = sscanf(line, "%s , , %s %s %s %s %s\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
        log.info("[%d] %s %s %s %s %s %s", ret, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
        sscanf(buf[2],"%lf", &ra);
        sscanf(buf[3],"%lf", &dec);
        sscanf(buf[4],"%lf", &exp);
        log.info("%f  %f  %f", ra, dec, exp);
    }
}

static void test_EOF() {
    FILE * pFile;
    int c;
    int n = 0;
    pFile = fopen("./test.txt", "r");
    if (pFile == NULL)
        perror("Error opening file");
    else {
        do {
            c = fgetc(pFile);
            printf("%02X", c);
            if (c == '$')
                n++;
        } while (c != EOF);
        fclose(pFile);
        printf("\nThe file contains %d dollar sign characters ($).\n", n);
    }
}

static void test_random() {
    srand(1);
    for (int i = 0; i < 5; i++) {
        srand(1);
        printf("%d\n", rand());
    }
    printf("\n");

    srand(1);
    for (int i = 0; i < 5; i++) {
        printf("%d\n", rand());
    }
    printf("\n");

    unsigned long int next = 1;
    next = next * 1103515245 + 12345;
    printf("%d\n", (unsigned int) (next / 65536) % 32768);

    printf("RAND_MAX: %lu\n", RAND_MAX);
}

static void test_stl_list() {
    std::list<std::pair<int, int>> mylist;
    std::list<std::pair<int, int>>::iterator iter;
    for (int i = 1; i <= 5; ++i) {
        mylist.push_back(std::make_pair(i, -i));
    }

    for (iter = mylist.begin(); iter != mylist.end(); ++iter) {
        printf("%d   %d\n", iter->first, iter->second);
    }

    iter = mylist.begin();
    iter++;
    iter->first = 21;

    for (iter = mylist.begin(); iter != mylist.end(); ++iter) {
        printf("%d   %d\n", iter->first, iter->second);
    }

}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Need a parameter of profile!");
        exit(1);
    }
    config.init(argv[1]);   // 初始化配置模块
    char *log_dir = config.get_string("LOG", "TestLogDir", "build/test_logs");
    log.init(log_dir);      // 初始化日志模块

//    test_read_file();

//    test_up_lower_bound();
//    test_read_data();
//    test_EOF();
//    assert(!test_double());
//    log.info("All tests passed!\n");
//    test_random();
    test_stl_list();

    return 0;
}
