#include "server.h"

#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "base.h"
#include "record.h"
#include "netypes.h"

static connection_manager_t conn_mgr;
static int socket_fd;
static stat_t stat;


// 设置文件描述符为非阻塞模式
bool set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return false;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) >= 0;
}

// 修改epoll事件
void mod_epoll_event(int epoll_fd, int fd, uint32_t events, connection_t *conn) {
    struct epoll_event ev;
    ev.events = events | EPOLLRDHUP;  // 总是监听连接关闭事件
    ev.data.ptr = conn;
    
    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev) < 0) {
        perror("epoll_ctl MOD");
        close_connection(&conn_mgr, conn);
    }
}


// 打印统计信息
void print_statistics(Record *r, int sz) {
    static time_t last_print = 0;
    time_t now = time(NULL);

    if (now - last_print >= 60) {  // 每10秒打印一次
        printf("\n=== 服务器统计 ===\n");
        printf("  数据条数     : %d\n",  r->size());
        printf("  数据占用空间 : %d\n",  sz * r->size());
        printf("  当前连接数   : %d\n",  conn_mgr.count);
        printf("  连接次数     : %lu\n",  stat.total);
        printf("  请求次数     : %lu\n",  stat.request);
        printf("  错误次数     : %lu\n",  stat.error);
        printf("  响应次数     : %lu\n",  stat.response);
        printf("  接收字节     : %lu\n", stat.recv_bytes);
        printf("  发送字节     : %lu\n", stat.send_bytes);

        last_print = now;
    }
}

bool opt_status(Record *r, buffer_t *send) {
    char *start = send->data + 8;
    char *p = start;
    p += setint8(p, conn_mgr.count); // 当前连接数
    p += setint8(p, stat.total); // 连接总数
    p += setint8(p, stat.request); // 请求总数
    p += setint8(p, stat.error); // 错误总数
    p += setint8(p, stat.response); // 响应总数
    p += setint8(p, stat.recv_bytes); // 接收字节总数
    p += setint8(p, stat.send_bytes); // 发送字节总数
    int sz = p - start;
    send->size = sz + 8;
    setint4(send->data, sz + 4);
    setint4(send->data+4, OK);
    return true;
}

Server::Server(Meta *m, Record *r) : meta_(m), record_(r) {
}

Server::~Server() {
    free_connection_manager(&conn_mgr);

    // 关闭监听套接字
    close(socket_fd);

    // 关闭epoll
    close(conn_mgr.epoll_fd);

    printf("[info] 服务器已正常关闭。\n");
}

bool Server::init() {
    // 创建服务器 socket
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        perror("socket creation failed");
        return false;
    }

    // 设置 SO_REUSEADDR 选项
    int opt = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        return false;
    }

    // 绑定地址和端口
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(meta_->port_);
    if (bind(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        return false;
    }
    
    // 开始监听
    if (listen(socket_fd, 128) < 0) {
        perror("listen failed");
        return false;
    }
    
    // 设置非阻塞
    if (!set_nonblocking(socket_fd)) {
        perror("set_nonblocking failed");
        return false;
    }

    // 初始化连接管理器
    if (!init_connection_manager(&conn_mgr, meta_->max_connections_)) {
        return false;
    }
    // 创建 epoll 实例
    conn_mgr.epoll_fd = epoll_create1(0);
    if (conn_mgr.epoll_fd < 0) {
        perror("epoll_create1 failed");
        return false;
    }
    
    // 添加监听套接字到epoll
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = socket_fd;
    if (epoll_ctl(conn_mgr.epoll_fd, EPOLL_CTL_ADD, socket_fd, &ev) < 0) {
        perror("epoll_ctl ADD listen_fd");
        return false;
    }

    printf("[info] epoll服务启动成功，端口=%d\n", meta_->port_);
    return true;
}

void Server::start(int *running) {
    struct epoll_event ev, events[MAX_EVENTS];
    // 事件循环
    while (1) {
        if (*running == 0) {
            int count = record_->dump();
            printf("[info] 数据保存成功（count=%d），准备退出\n", count);
            break;
        }
        int nfds = epoll_wait(conn_mgr.epoll_fd, events, MAX_EVENTS, 1000);
        if (nfds < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("epoll_wait");
            break;
        }

        // 处理事件
        for (int i = 0; i < nfds; i++) {
            // 新连接
            if (events[i].data.fd == socket_fd) {
                struct sockaddr_in client_addr;
                socklen_t addr_len = sizeof(client_addr);
                
                int client_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &addr_len);
                if (client_fd < 0) {
                    if (errno != EAGAIN && errno != EWOULDBLOCK) {
                        perror("accept");
                    }
                    continue;
                }
                
                // 检查连接数限制
                if (conn_mgr.count >= conn_mgr.max_count) {
                    printf("[warn] 连接数已达上限 %d，拒绝新连接\n", conn_mgr.max_count);
                    close(client_fd);
                    continue;
                }
                
                // 设置非阻塞
                set_nonblocking(client_fd);
                
                // 创建连接结构
                connection_t *conn = create_connection(client_fd, &client_addr);
                if (!conn) {
                    close(client_fd);
                    continue;
                }
                
                // 添加到连接管理器
                conn->id = conn_mgr.count;
                conn_mgr.connections[conn_mgr.count++] = conn;
                stat.total++;
                
                // 添加到epoll监听可读事件（水平触发），未采用边缘触发，因为我还无法完整读取超大数据
                ev.events = EPOLLIN | EPOLLRDHUP;
                ev.data.ptr = conn;
                if (epoll_ctl(conn_mgr.epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) < 0) {
                    perror("epoll_ctl ADD client_fd");
                    close_connection(&conn_mgr, conn);
                }
                
            } else {
                // 已有连接的事件
                connection_t *conn = (connection_t *)events[i].data.ptr;
                
                // 处理连接关闭或错误
                if (events[i].events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
                    printf("[info][fd=%d] 连接关闭\n", conn->fd);
                    close_connection(&conn_mgr, conn);
                    continue;
                }
                
                // 处理可读事件
                if (events[i].events & EPOLLIN) {
                    if (!handle_read_event(conn)) {
                        close_connection(&conn_mgr, conn);
                        continue;
                    }
                }
                
                // 处理可写事件
                if (events[i].events & EPOLLOUT) {
                    if (!handle_write_event(conn)) {
                        close_connection(&conn_mgr, conn);
                        continue;
                    }
                }
            }
        } // for events

        // 定期清理空闲连接（每秒一次）
        static time_t last_cleanup = 0;
        time_t now = time(NULL);
        if (now - last_cleanup >= 10) {
            cleanup_idle_connections(&conn_mgr, meta_->idle_timeout_, meta_->transport_timeout_);
            last_cleanup = now;
        }

        print_statistics(record_, meta_->record_size_);
    } // while

    printf("\n[info] 正在关闭服务器...\n");
}

bool Server::handle_read_event(connection_t *conn) {
    if (conn->state == STATE_IDLE) {
        // 空闲状态收到数据，开始处理新请求
        conn->state = STATE_READING_HEADER;
        conn->recv.offset = 0;
        conn->recv.size = 0;
    }
    
    int n = (int)read(conn->fd, conn->recv.data + conn->recv.offset, BUFFER_SIZE);
    //printf("read size %ld\n", n);
    
    if (n > 0) {
        // 成功读取数据
        conn->recv.offset += n;
        conn->last_active = time(NULL);
        conn->total_bytes_recv += n;
    } else if (n == 0) {
        // 对端关闭连接
        printf("[info][fd=%d] 对端关闭连接\n", conn->fd);
        return false;
    } else {
        // 读取错误
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // 数据已读完，等待下次事件
            ;
        } else if (errno == EINTR) {
            // 被信号中断，继续尝试
            ;
        } else {
            // 真正的错误
            perror("read error");
            return false;
        }
    }
    
    // 读到数据后，再根据状态处理
    if (conn->state == STATE_READING_HEADER) {
        stat.request++;
        // MY第一次读取该请求的数据
        if (!parse_header(conn)) {
            stat.error++;
            return false;  // 关闭连接
        }
    }

    // 检查是否接收完成
    if (conn->state == STATE_READING_BODY && conn->recv.offset >= conn->recv.size) {
        // 请求接收完成
        conn->request_count++;
        //printf("[info][fd=%d] 数据接收完成: 请求#%u, 大小=%d\n", conn->fd, conn->request_count, conn->recv.offset);
        stat.recv_bytes += conn->recv.offset;
        
        // 处理数据
        if (process_data(conn)) {
            // 准备发送响应数据
            conn->state = STATE_WRITING;
            conn->send.offset = 0;
            mod_epoll_event(conn_mgr.epoll_fd, conn->fd, EPOLLOUT, conn);
            return true;
        } else {
            printf("error process\n");
            return false;
        }
    }
    
    return true;
}

bool Server::parse_header(connection_t *conn) {
    // [4魔术数字][4数据长度][4操作类型][数据体]
    if (conn->recv.offset < HEADER_SIZE) {
        // 数据不够，继续监听等待
        return true;
    }
    header_t *header = (header_t *)conn->recv.data;
    if (header->magic != meta_->magic_number_) {
        printf("[warn][fd=%d] 魔术数字验证失败: 收到: %d, 期望: %d\n", conn->fd, header->magic, meta_->magic_number_);
        // 这里选择关闭连接，因为协议错误严重
        return false;
    }
    if (header->size < 0 || header->size + HEADER_SIZE > meta_->max_recv_size_) {
        printf("[warn][fd=%d] 数据大小超过限制: 请求大小=%d, 最大允许=%d\n", conn->fd, header->size, meta_->max_recv_size_);
        return false;
    }
    // 数据大小（头部+数据体）
    conn->recv.size = header->size + HEADER_SIZE;
    conn->state = STATE_READING_BODY;
    conn->opt = header->opt;

    // 一次性申请足够内存
    // 重新申请空间会对header有影响，所以在后面申请
    if (!expand_buffer(&conn->recv, conn->recv.size - conn->recv.offset)) {
        return false;
    }
    return true;
}

bool Server::process_data(connection_t *conn) {
    //printf("[info][fd=%d] receive opt:%d\n", conn->fd, conn->opt);
    try {
        switch (conn->opt) {
            case OPT_TEST:
                return record_->opt_test(&conn->recv, &conn->send);
            case OPT_CONF:
                return record_->opt_conf(&conn->recv, &conn->send);
            case OPT_STATUS:
                return opt_status(record_, &conn->send);
            case OPT_COLUMN:
                return record_->opt_column(&conn->recv, &conn->send);
            case OPT_DETAIL:
                return record_->opt_detail(&conn->recv, &conn->send);
            case OPT_DUMP:
                return record_->opt_dump(&conn->recv, &conn->send);
            case OPT_SET:
                return record_->opt_set(&conn->recv, &conn->send);
            case OPT_GET:
                return record_->opt_get(&conn->recv, &conn->send);
            case OPT_AGG:
                return record_->opt_agg(&conn->recv, &conn->send);
            case OPT_DEL:
                return record_->opt_del(&conn->recv, &conn->send);
            
            default:
                return false;
        }
    } catch (...) {
        return false;
    }
}

bool Server::handle_write_event(connection_t *conn) {
    if (conn->state != STATE_WRITING) {
        return true;
    }
    
    //printf("send buffer. size:%d offset:%d total:%d\n", conn->send.size, conn->send.offset, conn->send.total);
    int left = conn->send.size - conn->send.offset; // 剩余未发送数据
    if (left > BUFFER_SIZE) {
        left = BUFFER_SIZE;
    }
    int n = write(conn->fd, conn->send.data + conn->send.offset, left);
    if (n > 0) {
        conn->send.offset += n;
        conn->last_active = time(NULL);    
        conn->total_bytes_sent += n;
    } else if (n == 0) {
        // 写入0字节（通常不会发生）
        ;
    } else {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // 暂时无法写入，等待下次可写事件
            ;
        } else if (errno == EINTR) {
            ;
        } else {
            perror("write error");
            return false;
        }
    }
    
    //printf("[debug][fd=%d] 发送进度: 已发送=%d, 总计=%d n=%d\n", conn->fd, conn->send.offset, conn->send.size, n);
    
    if (conn->send.offset >= conn->send.size) {
        // 发送完成
        //printf("[info][fd=%d] 响应发送完成\n", conn->fd);
        stat.send_bytes += conn->send.size;
        stat.response++;
        
        // 重置状态，准备接收下一个请求（保持连接）
        conn->state = STATE_IDLE;
        // 修改epoll事件为监听可读事件
        mod_epoll_event(conn_mgr.epoll_fd, conn->fd, EPOLLIN, conn);
    }
    
    return true;
}
