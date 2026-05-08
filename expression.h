/*
 * Copyright (c) 2025-2035, shekunlong <ruc_skl@163.com>
 * All rights reserved.
 *
 * 表达式(Expression) & 公式计算(Computer)
 *      结果返回整数(int64_t)或浮点数(double)
 * 支持：
 *      (1) 运算符：  + - * / %
 *      (2) 优先级：  ()
 * 例如：
 *      00: a
 *      01: a + b
 *      02: (a - b) / b * 100
 *      03: a % 10
 *      04: (a+b)*(c-d)
 *
 */

#ifndef EXPRESSION_H_
#define EXPRESSION_H_

#include <stdlib.h>
#include <string>
#include <vector>

using namespace std;

namespace expr {

enum Type {
    kand = 1,
    kor,

    kleft,  // (
    kright, // )

    kadd,   // +
    ksub,   // -
    kmulti, // *
    kdiv,   // /
    kmod,   // %

    keq,    // =
    kgt,    // >
    kge,    // >=
    klt,    // <
    kle,    // <=
    kne,    // !=

    klike,  // 关键字：like
    knlike, // 关键字：notlike 

    kname,  // 字段名
    kid,    // 字段ID，需要meta信息
    kint1,  // 整数，1字节
    kint2,  // 整数，2字节
    kint4,  // 整数，4字节
    kint8,  // 整数，8字节
    kfloat, // 浮点，单精度，4字节
    kdouble,// 浮点，双精度，8字节
    kchar,  // 字符串

    kexpr,  // 表达式，暂时无用
    kerror, // 错误
};

static const char *gtype[] = {
    "", 
    "and", "or", 
    "(", ")", 
    "+", "-", "*", "/", "%", 
    "=", ">", ">=", "<", "<=", "!=", 
    "like", "notlike", 
    "kname", "kid", "kint1", "kint2", "kint4", "kint8", "kfloat", "kdouble", "kchar", 
    "kexpr", "kerror"
};

// 比较符号
bool is_compare_type(Type type);

// like notlike
bool is_like_type(Type type);

// 四则运算
bool is_four_type(Type type);

// and or
bool is_logical_type(Type type);

// 数字类型
bool is_digit_type(Type type);

struct Node {
    Type        type;
    int         id; // 字段ID，预留字段
    double      vd;
    string      vs;
};

typedef vector<Node*> Expression;

// 从字符串中读取成表达式Expression
// 如果返回失败，需要调用free_expression，释放申请的空间
bool load_expression(const char *str, int size, Expression *ex, string *errmsg);

// 检查表达式是否有效，有效返回true
// （1）类型仅限于：小括号、运算符、字段、数字（不能是字符串）
// （2）小括号需要配对
// （3）运算符两边要有操作数
bool check_expression(const Expression& ex, string *errmsg);

// 打印输出表达式
void print_expression(const Expression& ex);

// 释放表达式中的Node*
void free_expression(const Expression& ex);


/*
 * 公式计算
 *
 */

// 与Node不同在于，缺少一个string字段
struct CNode {
    Type    type;
    double  v;
    CNode() {}
    CNode(Type t) : type(t) {}
    CNode(Type t, double d) : type(t), v(d) {}
};

// 根据公式计算表达式，数据采用double存储，注意数字溢出。
// 默认公式有效
class Computer {
    public:
        Computer();
        ~Computer();
        void init(); // 计算前必须调用
        void push_opt(Type t);
        void push_number(double v);
        double result();

    private:
        // data数据有3种：1括号，2运算符，3浮点数字
        vector<CNode> data;
};

} // namespace expr

#endif // EXPRESSION_H_
