/*
 * config.h
 *
 *  Created on: Oct 22, 2016
 *      Author: sunchao
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include "monitor.h"

class Config {

private:
    char *_dir;         // 配置文件所在的文件夹
    char *_filename;    // 配置文件的文件名
    void *_config;      // 内存存储配置信息
    Monitor _monitor;

    /* 清空内存中的配置信息。 */
    void clear_config();

public:
    Config();
    ~Config();

    /* 读取配置文件（无dir信息）。如果已经读取，则将之前数据清空。 */
    void read_config(const char *filename);

    /* 初始化配置模块，读取配置文件。如果已经读取，则将之前数据清空。 */
    /* initialize the Config object, read configuration from file. */
    void init(const char *filepath);

    /* 读取INT类型的配置信息，需要默认值。 */
    /* read integer value in the Config object. */
    int get_int(const char *sec, const char *key, const int deft);

    /* 读取字符串类型的配置信息，需要默认值。 */
    /* read string value in the Config object. */
    char* get_string(const char *sec, const char *key, const char *deft);

    /* 读取DOUBLE类型的配置信息，需要默认值。 */
    /* read double value in the Config object. */
    double get_double(const char *sec, const char *key, const double deft);

    /* 获取配置文件路径。 */
    char *get_config_path();

    /* 打印所有配置，用于调试。 */
    void print_all();
};

/* 全局变量，便于读取配置信息。 */
extern Config config;

#endif /* CONFIG_H_ */
