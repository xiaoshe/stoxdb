#类的调用关系
Meta -> Record -> Server
Computer -> Record


####

- base.h：基础函数

- expression.h：表达式与公式计算

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


