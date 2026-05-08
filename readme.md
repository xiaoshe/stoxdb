# 介绍
StoxDB是一个全内存的 Key-Value 数据库，专门用来存储实时行情数据。它支持配置公式自动计算字段，也支持按条件查询。
一支股票一条数据：
- key：长度不要超过64（超过64的丢弃全部数据），使用明文（非字节流），区分大小写，建议使用股票代码，例如000001.SZ
- value：字段集合。

# 使用说明
## 服务端
- 按自己的需求编写配置文件db.conf
- 启动服务：nohup ./stoxdb &

## 客户端（python）
```python
import stoxdb
xdb = stoxdb.Client(host=..., port=..., magic=...)
```
- 添加和修改
```python
xdb.set("000001.SZ", {"code":"000001.SZ", "name":"平安银行", "incr":1.23})
```
- 查询一：构造请求体
```python
# （1）按key查询
req = stoxdb.Request()
req.fields = "code,name,incr"
req.keys = "000001.SZ"
for x in xdb.get(req):
    print(x)
# 输出：['000001.SZ', '平安银行', 1.23]

# （2）按key查询，输出字典格式
req.fmt = "{}"
for x in xdb.get(req):
    print(x)
# 输出：{'code': '000001.SZ', 'name': '平安银行', 'incr': -0.26}

# （3）输出涨幅大于3的股票top10
req = stoxdb.Request()
req.fields = "code,name,incr"
req.where = "incr>3" # 条件
req.size = 10  # top10
req.order = "incr desc" # 排序字段
req.fmt = "{}"
for x in xdb.get(req):
    print(x)
# 输出：
#  {'code': '688655.SH', 'name': '迅捷兴', 'incr': 20.0}
#  {'code': '300736.SZ', 'name': '百邦科技', 'incr': 20.0}
#  {'code': '920036.BJ', 'name': '觅睿科技', 'incr': 19.32}
#  {'code': '301369.SZ', 'name': '联动科技', 'incr': 15.92}
#  {'code': '300840.SZ', 'name': '酷特智能', 'incr': 14.5}
#  {'code': '301603.SZ', 'name': '乔锋智能', 'incr': 12.8}
#  {'code': '300052.SZ', 'name': '中青宝', 'incr': 12.75}
#  {'code': '300819.SZ', 'name': '聚杰微纤', 'incr': 12.47}
#  {'code': '000584.SZ', 'name': 'ST工智', 'incr': 11.54}
#  {'code': '920270.BJ', 'name': '天铭科技', 'incr': 11.46}
```

- 查询二：sql
```python
# 输出涨幅大于3的股票top10，字典格式输出
sql = "select code,name,incr where incr>3 order by incr desc limit 10"
for x in xdb.select(sql, True):
    print(x)
# 输出同
```
- 查询条件：
-- 支持like，notlike（not与like中间没有空格）
-- like字符串支持^开头、$结尾符号，例如，like '^ST'
-- like字符串不支持大小写，例如，like 'ST' 与 like 'st' 返回结果不一样
-- 不支持in

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