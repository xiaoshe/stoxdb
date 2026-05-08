#ifndef BUFFER_H_
#define BUFFER_H_

/*
 * buffer_t
 *  （1）数据写入时，会扩充空间，需要使用offset确定写入位置
 *      所以，剩余空间 = capacity - offset
 *
 *  （2）数据读取时，offset从0开始，到size结束。
 *
 *  在网络传输时，size是已知的。offset表示接收（发送）量，当offset==size时，接收（发送）结束。
 */

typedef struct {
    char *data;        // 缓冲区指针
    int   capacity;    // 缓冲区大小

    int   offset;      // 接收数据量、发送数据量
    int   size;        // 数据大小（头部+数据体），offset <= total <= size
} buffer_t;



// 初始化buffer_t空间，空间不足返回false
bool init_buffer(buffer_t *bf, int size);

// 释放buffer_t空间
bool free_buffer(buffer_t *bf);

// 扩展buffer_t空间，offset已使用
bool expand_buffer(buffer_t *bf, int addsize);

#endif
