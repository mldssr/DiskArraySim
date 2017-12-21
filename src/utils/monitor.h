/*
 * monitor.h
 *
 *  Created on: Jan 10, 2017
 *      Author: sunchao
 */

#ifndef MONITOR_H_
#define MONITOR_H_

/* 文件系统发生改变时，调用的通知函数。如果filename为NULL，则为注销操作。 */
typedef void (*notify_t)(const char *filename, void *params);

class Monitor {
    int _monitor;       // 监控器描述符
    int _watcher;       // 监控单元标志
    char *_path;        // 监控的文件夹
    char *_pattern;     // 监控特定格式的文件
    notify_t _notity;   // 回调函数
    void *_params;      // 回调函数需要的参数

public:
    Monitor();
    ~Monitor();

    /* 监控特定文件夹下的文件，文件名可用*.ext表示。 */
    void set(const char *path, const char *pattern, notify_t notify, void *params);

    /* 对当前文件中的现有文件进行操作。 */
    void walk(notify_t notify, void *params);

    /* 调用监控操作，只有在被调用时才去处理文件改动。 */
    void check();

    /* 停止监控。 */
    void stop();
};

#endif /* MONITOR_H_ */
