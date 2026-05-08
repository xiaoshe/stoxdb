#ifndef DATYPES_H_
#define DATYPES_H_

#include "expression.h"

using namespace std;
using namespace expr;

struct Field {
    string  name;       // 字段名称
    Type    type;       // 类型
    string  note;       // 备注
    Expression formula;    // 计算公式
    vector<int> decide; // 该字段修改会影响某些字段，由formula确定
    int     offset;     // 数据在内存中的偏移位置
    int     size;       // 数据大小

    string  rtype;      // 原始类型，配置文件
    string  rform;      // 原始公式，配置文件
};

#endif // DATYPES_H_
