#类的调用关系
Meta -> Record -> Server
Computer -> Record


expression.h  支持in like

expression.h  去掉in，list<Node>，LogicNode，表达式计算

/*
 * 实验对比：每次push_back() n条Node数据，共运行100万次
 * 实验结果：
 *                      n=50      n=16    n=8     n=4     n=4
 *      list<Node>    : 9177 ms   2811    1276    891     649
 *      list<Node*>   : 7097 ms   2332    1151    624     600
 *      vector<Node>  : 6705 ms   3508    1537    1112    334 reserve(16)
 *      vector<Node*> : 3036 ms   1927    1331    961     255 reserve(16)
 * n越大vector越优，n越小list越优
 * 结论：采用vector，使用reserve(16)，提前预支16个长度空间，速度最快
 */



####

base.h
    基础函数

expression.h
    表达式与公式计算

logical.h
    逻辑判断:and or

datypes.h
    数据结构，字段属性，表达式

netypes.h
    数据结构，用于网络传输

connection.h
    链接管理

meta.h
    配置管理

record.h
    数据管理，读取和写入，由server.h调用

server.h
    epoll网络模型，主流程


