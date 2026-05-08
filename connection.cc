
#include "connection.h"

// 创建连接结构
connection_t *create_connection(int fd, struct sockaddr_in *addr) {
    connection_t *conn = (connection_t*)calloc(1, sizeof(connection_t));
    if (!conn) {
        perror("calloc connection");
        return NULL;
    }
    
    conn->fd = fd;
    conn->state = STATE_IDLE;
    conn->last_active = time(NULL);
    conn->start_time = time(NULL);
    conn->request_count = 0;
    conn->error_count = 0;
    conn->total_bytes_recv = 0;
    conn->total_bytes_sent = 0;
    
    // 复制客户端地址
    if (addr) {
        memcpy(&conn->addr, addr, sizeof(struct sockaddr_in));
        inet_ntop(AF_INET, &addr->sin_addr, conn->ip_str, INET_ADDRSTRLEN);
    }
    
    // 初始化接收缓冲区（初始8KB）
    if (!init_buffer(&conn->recv, BUFFER_SIZE)) {
        free(conn);
        perror("malloc recv buffer");
        return NULL;
    }
    
    // 初始化发送缓冲区（初始8KB）
    if (!init_buffer(&conn->send, BUFFER_SIZE)) {
        free_buffer(&conn->recv);
        free(conn);
        perror("malloc send buffer");
        return NULL;
    }
    
    //printf("[info][fd=%d] 新连接, IP=%s:%d\n", fd, conn->ip_str, ntohs(addr->sin_port));
    return conn;
}

void print_connection(connection_t *conn) {
    printf("[info] connection: fd=%d state=%d recvbuf=%d\n", conn->fd, conn->state, conn->recv.capacity);
}


// 释放连接资源
void free_connection(connection_t *conn) {
    if (!conn) return;
    
    free_buffer(&conn->recv);
    free_buffer(&conn->send);
    
    printf("[info][fd=%d] 释放连接: IP=%s, 请求数=%u, 错误数=%u, 收到=%lu，发送=%lu\n", 
           conn->fd, conn->ip_str, conn->request_count, 
           conn->error_count, conn->total_bytes_recv, conn->total_bytes_sent);
    free(conn);
}

// 关闭连接
void close_connection(connection_manager_t *mgr, connection_t *conn) {
    if (!conn) return;
    
    // 从epoll中删除
    if (conn->fd > 0) {
        epoll_ctl(mgr->epoll_fd, EPOLL_CTL_DEL, conn->fd, NULL);
    }
    
    // 关闭套接字
    if (conn->fd > 0) {
        close(conn->fd);
    }
    
    // TODO
    // 从连接管理器中移除
    /*
    for (size_t i = 0; i < mgr->count; i++) {
        if (mgr->connections[i] == conn) {
            // 最后一个覆盖当前
            mgr->connections[i] = mgr->connections[mgr->count - 1];
            mgr->count--;
            break;
        }
    }
    */

    // 直接根据id删除
    int id = conn->id;
    connection_t *last = mgr->connections[mgr->count - 1];
    last->id = id;
    mgr->connections[id] = last;
    mgr->count--;
    
    free_connection(conn);
}


bool init_connection_manager(connection_manager_t *mgr, int max_count) {
    // 初始化连接管理器
    mgr->max_count = max_count;
    mgr->count = 0;
    mgr->connections = (connection_t**)calloc(mgr->max_count, sizeof(connection_t *));
    if (!mgr->connections) {
        perror("calloc connections");
        return false;
    }
    return true;
}

void free_connection_manager(connection_manager_t *mgr) {
    // 关闭所有连接
    for (int i = 0; i < mgr->count; i++) {
        connection_t *conn = mgr->connections[i];
        if (conn == NULL) continue;

        // 从epoll中删除
        if (conn->fd > 0) {
            epoll_ctl(mgr->epoll_fd, EPOLL_CTL_DEL, conn->fd, NULL);
        }

        // 关闭套接字
        if (conn->fd > 0) {
            close(conn->fd);
        }

        free_connection(conn);
    }
    // 释放连接管理器
    free(mgr->connections);
}

void cleanup_idle_connections(connection_manager_t *mgr, int idle_timeout, int recv_timeout) {
    time_t now = time(NULL);

    for (size_t i = 0; i < mgr->count; i++) {
        connection_t *conn = mgr->connections[i];
        if (!conn) continue;

        // 检查空闲超时（只有在空闲状态才检查）
        if (conn->state == STATE_IDLE) {
            if (now - conn->last_active > idle_timeout) {
                printf("[info][fd=%d] 空闲连接超时关闭: IP=%s, 空闲时间=%ld秒\n",
                       conn->fd, conn->ip_str, now - conn->last_active);
                close_connection(mgr, conn);
                i--;  // 因为数组被调整
            }
        }
        // 检查接收超时（防止客户端发送过慢）
        else if (conn->state == STATE_READING_HEADER ||
                 conn->state == STATE_READING_BODY ||
                 conn->state == STATE_WRITING) {
            if (now - conn->last_active > recv_timeout) {
                printf("[warn][fd=%d] 操作超时关闭: IP=%s, 状态=%d, 超时时间=%ld秒\n",
                       conn->fd, conn->ip_str, conn->state, now - conn->last_active);
                close_connection(mgr, conn);
                i--;
            }
        }
    }
}
