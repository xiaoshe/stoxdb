/*
 * Copyright (c) 2025-2035, shekunlong <ruc_skl@163.com>
 * All rights reserved.
 *
 * 逻辑判断(Logical)
 *
 * 逻辑判断，返回true或false
 * 类似：(a and b) or c or (d and e)
 * 支持：
 *      (1) 逻辑符号：and, or
 *      (2) 优先级：  ()
 *      (3) 表达式：  
 *      (4) 比较符：  > < = >= <= !=
 *      (5) 匹配：    like, not like
 *
 * 例如：
 *      00: a > 1.2
 *      01: a < b
 *      02: a + b >= c/e
 *      03: a like '%logic'
 *      04: a > 0 and b = 1
 *      05: (a>0 and b=1) or c<d
 *      06: (a+b-c)*d/(e - 1) >= f and a-4<-5 and (b=1 or b=-2 or b=3) AND c NOT LIKE '^ST'  and   PROFIT>10000000000 and (a=1 or b>2)
 *
 * TODO：
 *      (1) 是否存在逻辑错误
 *      (2) 字段是否存在
 *      (3) 字段类型是否合理，数字类型不能用于匹配（like），字符类型不能用于运算（加减乘除）
 *      (3) 计算公式，存在小数则用double，否则用int64_t
 *      (4) 逻辑比较
 *      (5) 
 */

#ifndef LOGICAL_H_
#define LOGICAL_H_

#include <stdlib.h>
#include "expression.h"

namespace expr {

// 逻辑节点
struct LogicalNode {
    Type        type; // 共5种情况：and or ( ) 表达式
    Type        extype; // 表达式的类型：kdouble浮点, kint8整数, kchar字符串, kerror错误（数字与字符串均有）
    bool        like_start; // 是否以xx开头
    bool        like_end;   // 是否以xx结尾
    Expression  left;
    Expression  right;
};

typedef vector<LogicalNode*> Logical;

// 节点转化：Node ---> LogicalNode，输入节点被释放
LogicalNode *convert(Node *in);


// 先读取成Expression，再组装成Logical
bool load_logical(const char *str, int size, Logical *lg, string *errmsg);

void print_logical(const Logical& lg);

bool check_logical(const Logical& lg, string *errmsg);

void free_logical(Logical& lg);

}

#endif // LOGICAL_H_
