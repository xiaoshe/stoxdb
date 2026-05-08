#include <malloc.h>
#include "buffer.h"

// 初始化buffer_t空间
bool init_buffer(buffer_t *bf, int size) {
    bf->capacity = size;
    bf->offset = 0;
    bf->size = 0;
    bf->data = (char *)malloc(size);
    if (bf->data == NULL) {
        // 内存不足，啥也干不了，不如直接死了
        bf->capacity = 0;
        return false;
    }
    return true;
}

// 释放buffer_t空间
bool free_buffer(buffer_t *bf) {
    if (bf->data) free(bf->data);
    bf->capacity = 0;
    return true;
}

// 扩展buffer_t空间
bool expand_buffer(buffer_t *bf, int addsize) {
    if (addsize <= 0) return true;
    int capacity = bf->capacity;
    while (capacity < bf->offset + addsize) {
        // 剩余空间不足，需要扩充内存，翻倍扩充内存
        capacity = capacity * 2;
    }
    if (capacity > bf->capacity) {
        char *newbuf = (char *)realloc(bf->data, capacity);
        if (!newbuf) {
            printf("内存不足. size:%d\n", capacity);
            return false;
        }
        printf("expand buff. %d -> %d\n", bf->capacity, capacity);
        bf->data = newbuf;
        bf->capacity = capacity;
    }
    return true;
}

