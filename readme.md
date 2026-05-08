# 介绍
# 使用说明




# 文件说明
- base.h：基础函数
- datypes.h：数据结构Field，用于配置文件
- netypes.h：数据结构，用于网络传输
- meta.h：配置信息管理
- expression.h：表达式（数据结构Expression）与公式计算（类Computer）
- connection.h：链接池管理：添加、移除链接
- buffer.h：缓冲区操作：申请、释放、扩展内存空间
- logical.h：逻辑判断，例如：(a and b) or c or (d and e)
- record.h：数据管理：读取和写入，处理客户端的请求
- server.h：epoll网络模型
- main.cc：主流程

## 类的调用关系
- Meta     -> Record -> Server
- Computer -> Record