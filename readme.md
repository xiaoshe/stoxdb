# 介绍
StoxDB是一个全内存的 Key-Value 数据库，专门用来存储实时行情数据。它支持配置公式自动计算字段，也支持按条件查询。
一支股票一条数据：
- key：长度不要超过64（超过64的丢弃全部数据），使用明文（非字节流），区分大小写，建议使用股票代码，例如000001.SZ
- value：字段集合。

# 使用说明
## 服务端
- 按自己的需求编写配置文件db.conf
- 启动服务：
<code>nohup ./stoxdb &</code>
## 客户端（python）
<code>import stoxdb
db = stoxdb.Client(host=..., port=..., magic=...)
</code>
- 添加和修改
<code>db.set("000001.SZ", {"name":"平安银行", "price":10.12})
</code>

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