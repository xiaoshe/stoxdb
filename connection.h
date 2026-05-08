#ifndef CONNECTION_H_
#define CONNECTION_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <stdint.h>

#include "netypes.h"
#include "buffer.h"


// 连接结构体
typedef struct {
    int id;                    // 链接ID，从0开始
    int fd;                    // 套接字描述符
    enum conn_state state;     // 当前状态
    int opt;                   // 操作类型
    
    // 接收缓冲区
    buffer_t recv;
    // 发送缓冲区
    buffer_t send;
    
    // 时间管理
    time_t last_active;       // 最后活跃时间
    time_t start_time;        // 连接开始时间
    
    // 统计信息
    uint32_t request_count;     // 已处理的请求数
    uint32_t error_count;       // 错误请求数
    uint64_t total_bytes_recv;  // 总共接收的字节数
    uint64_t total_bytes_sent;  // 总共发送的字节数
    
    // 其他信息
    struct sockaddr_in addr;  // 客户端地址
    char ip_str[INET_ADDRSTRLEN];  // IP字符串
} connection_t;

// 连接管理器
typedef struct {
    connection_t **connections;  // 连接数组
    int count;                   // 当前连接数
    int max_count;               // 最大连接数
    int epoll_fd;                // epoll文件描述符
} connection_manager_t;

void print_connection(connection_t *conn);
// 为每一个客户端创建一个链接，并申请接收、发送缓存
connection_t *create_connection(int fd, struct sockaddr_in *addr);
// 关闭某个链接，并释放申请的空间
void close_connection(connection_manager_t *mgr, connection_t *conn);

bool init_connection_manager(connection_manager_t *mgr, int max_count);
void free_connection_manager(connection_manager_t *mgr);
void cleanup_idle_connections(connection_manager_t *mgr, int idle_timeout, int recv_timeout);
#endif // CONNECTION_H_
