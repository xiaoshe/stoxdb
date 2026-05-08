#ifndef NETYPES_H_
#define NETYPES_H_

#include <stdlib.h>

#define BUFFER_SIZE 8192

// 连接状态枚举
enum conn_state {
    STATE_READING_HEADER,   // 读取头部（12字节）
    STATE_READING_BODY,     // 读取数据体
    STATE_WRITING,          // 写入响应
    STATE_IDLE              // 空闲，等待下一个请求
};


#define MAX_EVENTS 1024
#define HEADER_SIZE 12

// 响应成功OK，错误ERROR
#define OK 0
#define ERROR 1

// 协议头部定义
typedef struct {
    int magic;       // 魔术数字
    int size;        // 网络字节序的数据长度，不含这12字节
    int opt;         // 操作数
} header_t;


// 读写操作类型
enum operation {
    OPT_TEST    = 1,    // 测试，数据大小、魔术数字、接收发送等待
    OPT_CONF    = 2,    // 获取配置信息，夫服务器配置+字段+数据
    OPT_COLUMN  = 3,    // 获取字段信息，字段名+字段ID，更新时需要指定字段ID
    OPT_DETAIL  = 4,    // 获取字段详细信息
    OPT_STATUS  = 5,    // 获取服务器当前状态

    OPT_DUMP    = 10,   // 备份服务器数据
    OPT_SET     = 11,   // 数据更新，数据存在则更新，不存在则添加
    OPT_GET     = 12,   // 数据读取，类似SQL中的select
    OPT_AGG     = 13,   // 数据统计，聚合操作
    OPT_DEL     = 14,   // 数据删除
};

typedef struct {
    uint64_t total;         // 链接总数
    uint64_t request;       // 请求总数
    uint64_t error;         // 错误总数
    uint64_t response;      // 响应总数
    uint64_t recv_bytes;    // 接收字节总数
    uint64_t send_bytes;    // 发送字节总数
} stat_t;



#endif // NETYPES_H_
