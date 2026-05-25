import socket
import struct
import time
import math
from enum import Enum

class Operation(Enum):
    TEST   = 1
    CONF   = 2
    COLUMN = 3
    DETAIL = 4
    STATUS = 5

    DUMP = 10
    SET  = 11
    GET  = 12
    AGG  = 13
    DEL  = 14

class Type(Enum):
    kint1   = 20
    kint2   = 21
    kint4   = 22
    kint8   = 23
    kfloat  = 24
    kdouble = 25
    kchar   = 26

def Int(v):
    try:
        return int(v)
    except:
        return 0
def Float(v):
    try:
        return float(v)
    except:
        return 0

class Request:
    """
        fields: 字段列表，逗号分割，例如：code,name,open,close,high,low
        keys  : 指定key，逗号分割，例如：000001.SZ,000001.SH
        where : 查询条件，例如：incr>2 and pe<50
        order : 排序方式，例如：incr desc, pe asc
        aggs  : 聚合函数，逗号分割，例如：count(1),max(incr),avg(close)
        group : 按某些字段分组，逗号分割，例如：type,code
        offset: 数据返回位置，默认0
        size  : 返回条数，-1全部，默认-1
        fmt   : 数据返回格式，只有两种：[]/{}，默认[]
    """
    fields : str = ""
    keys   : str = ""
    where  : str = ""
    order  : str = ""
    aggs   : str = ""
    group  : str = ""
    offset : int = 0
    size   : int = -1
    fmt    : str = "[]"

class Client:
    sock = None
    host = ""
    port = 0
    magic = 0
    errcode = 0
    connected = False
    fields = {} # name => [id,type,size]

    
    def __init__(self, host:str="127.0.0.1", port:int=30634, magic:int=41401891):
        self.host = host
        self.port = port
        self.magic = magic

        self.__connect()

    def __del__(self):
        self.close()

    def __connect(self) -> bool:
        while True:
            try:
                self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                self.sock.connect((self.host, self.port))
                self.connected = True
                print("connected {}:{}".format(self.host, self.port))
                self.__update_fields()

                return True
            except Exception as e:
                print(e, "{}:{}".format(self.host, self.port))
                time.sleep(1)

        return False

    def close(self):
        if self.sock:
            self.sock.close()


    # 请求0
    def test(self):
        """
        仅用于测试请求和响应，返回的数据结构与其他不一样
        """
        buff = bytes(4*1024*1024-12) # 最大4*1024*1024-12

        magic = 41401891
        opt = 1
        size = len(buff)
        data = struct.pack("iii", magic, size, opt)

        self.sock.sendall(data + buff)

        buff = self.sock.recv(12)
        magic,size,opt = struct.unpack("iii", buff)
        print(magic, size, opt)
        data = b""
        while len(data) < size:
            buff = self.sock.recv(size-len(data))
            if not buff:
                break
            data += buff
        print(data.decode())


    def __send(self, opt, buff:bytes) -> bool:
        size = len(buff)
        data = struct.pack("iii", self.magic, size, opt)

        while True:
            try:
                self.sock.sendall(data + buff)
                return True
            except Exception as e:
                print(e)
                time.sleep(1)
        return False


    def __recv(self) -> bytes:
        buff = self.sock.recv(8)
        size,self.errcode = struct.unpack("ii", buff)
        #print("[info] recv ok. size:{} errcode:{}".format(size, self.errcode))
        size -= 4

        data = b""
        while len(data) < size:
            chunk = self.sock.recv(size - len(data))
            if not chunk:
                break
            data += chunk
        return data

    # 当链接断开时，重试
    def __send_and_recv(self, opt, buff:bytes) -> bytes:
        while True:
            try:
                self.__send(opt, buff)
                resp = self.__recv()
                return resp
            except:
                time.sleep(1)
                self.__connect()

    def __check_error(self, buff):
        if self.errcode != 0:
            l, = struct.unpack("h", buff[0:2])
            errmsg = buff[2:]
            print("[error] code:{} msg:{}".format(self.errcode, errmsg.decode()))
            return True
        else:
            return False


    # 请求
    def conf(self) -> dict:
        """
        获取服务器配置信息
        """
        resp = self.__send_and_recv(Operation.CONF.value, b"")
        if self.__check_error(resp):
            return {}
        
        port,l = struct.unpack("ib", resp[0:5])
        dumpfile = resp[5:5+l]
        x = struct.unpack("iiiiii", resp[5+l:])

        return {
            "port":port,
            "dumpfile":dumpfile.decode(),
            "magic":x[0],
            "max_recv_size":x[1],
            "max_connections":x[2],
            "record_size":x[3],
            "field_count":x[4],
            "data_count":x[5],
        }

    # 请求2
    def status(self) -> dict:
        """
        获取服务器当前状态
        """
        resp = self.__send_and_recv(Operation.STATUS.value, b"")
        if self.__check_error(resp):
            return {}

        x = struct.unpack("qqqqqqq", resp)
        return {
            "current_connect_number":x[0],
            "connected_number":x[1],
            "request_number":x[2],
            "request_error_number":x[3],
            "response_number":x[4],
            "recv_bytes":x[5],
            "send_bytes":x[6]
        }

    # 请求3
    def column(self) -> list:
        """
        获取字段信息，返回: [ ... [字段名、类型、大小] ... ]）
        """
        resp = self.__send_and_recv(Operation.COLUMN.value, b"")
        if self.__check_error(resp):
            return []

        data = []
        while resp:
            tp,sz,l = struct.unpack("<bhb", resp[0:4]) # 小端
            name = resp[4:4+l]
            data.append([name.decode(), tp, sz])
            resp = resp[4+l:]

        return data
    
    # 链接后请求所有字段
    def __update_fields(self):
        data = self.column()
        self.fields = {}
        i = 0
        for name,tp,size in data:
            self.fields[name] = [i, tp, size]
            i += 1
        #print(self.fields)


    # 请求4
    def detail(self) -> list:
        """
        获取字段信息，返回: [ ... [字段名、类型、备注、公式] ... ]）
        """
        resp = self.__send_and_recv(Operation.DETAIL.value, b"")
        if self.__check_error(resp):
            return []

        data = []
        while resp:
            l, = struct.unpack("b", resp[0:1])
            name = resp[1:1+l]
            resp = resp[1+l:]
            l, = struct.unpack("b", resp[0:1])
            rtype = resp[1:1+l]
            resp = resp[1+l:]
            l, = struct.unpack("h", resp[0:2])
            note = resp[2:2+l]
            resp = resp[2+l:]
            l, = struct.unpack("h", resp[0:2])
            rform = resp[2:2+l]
            resp = resp[2+l:]
            data.append([name.decode(), rtype.decode(), note.decode(), rform.decode()])

        return data

    # 请求5
    def dump(self) -> int:
        """
        保存数据
        """
        resp = self.__send_and_recv(Operation.DUMP.value, b"")
        if self.__check_error(resp):
            return 0

        size, = struct.unpack("i", resp[0:4])
        return size

    # 请求6
    def set(self, key:str, value:dict) -> int:
        """
        更新一条数据，返回更新成功条数
        """
        return self.setmore([[key, value]])

    # 请求7
    def setmore(self, data:list) -> int:
        """
        更新多条数据，返回更新成功条数
        @data: [ ... [key, value] ... ]
        """
        # 构造请求buff
        buff = b""
        for key,value in data:
            if len(key) > 64:
                print(f"[error]key太长：{key}")
                return 0

            # 先key
            buff1 = struct.pack("h", len(key))
            buff1 += key.encode()
            
            # 后value
            for name,v in value.items():
                if v is None:
                    # 跳过
                    continue

                name = name.lower()
                if name not in self.fields:
                    print(f"字段不存在. {name}")
                    return 0
                
                fid,ftype,fsize = self.fields[name]
                buff1 += struct.pack("h", fid)
                if ftype == Type.kchar.value:
                    # 字符串
                    if type(v) != type(""):
                        v = ""
                    while True:
                        a = v.encode()
                        if len(a) > fsize:
                            v = v[:-1]
                        else:
                            break
                    v = a
                    buff1 += struct.pack("h", len(v))
                    buff1 += v
                elif ftype == Type.kint1.value:
                    buff1 += struct.pack("b", Int(v))
                elif ftype == Type.kint2.value:
                    buff1 += struct.pack("h", Int(v))
                elif ftype == Type.kint4.value:
                    buff1 += struct.pack("i", Int(v))
                elif ftype == Type.kint8.value:
                    buff1 += struct.pack("q", Int(v))
                elif ftype == Type.kfloat.value:
                    buff1 += struct.pack("f", Float(v))
                elif ftype == Type.kdouble.value:
                    buff1 += struct.pack("d", Float(v))
                else:
                    print(f"类型错误. {name}:{ftype}")

            buff += struct.pack("i", len(buff1)) + buff1

        # 发送buff
        resp = self.__send_and_recv(Operation.SET.value, buff)
        if self.__check_error(resp):
            return []

        # 响应
        size, = struct.unpack("i", resp[0:4])
        return size


    # 请求8：读取数据
    def get(self, req:Request) -> list:
        """
        读取数据
        """
        buff = b""

        # (1)字段ID列表、名字列表
        fids = []
        fout = [] # 用于输出
        if "*" in req.fields:
            for name,v in self.fields.items():
                # v = [id,type,size]
                fids.append(v[0])
                fout.append([v[0], name, v[1], v[2]])
        else:
            for name in req.fields.split(","):
                name = name.strip().lower()
                if name not in self.fields:
                    print(f"[error] 字段不存在: {name}")
                    return []
                fid,ftype,fsize = self.fields[name]
                fids.append(fid)
                fout.append([fid, name, ftype, fsize])

        if not fids:
            print("[error] 无字段")
            return []

        buff += struct.pack("h", len(fids))
        for x in fids:
            buff += struct.pack("h", x)

        # (2)关键字
        keys = req.keys.strip()
        keys = keys.split(",") if keys else []
        buff += struct.pack("h", len(keys))
        for x in keys:
            x = x.strip()
            if len(x) > 64:
                print(f"[error]key太长:{x}")
                return []
            buff += struct.pack("h", len(x))
            buff += x.encode()

        # (3)查询条件
        where = req.where.encode()
        buff += struct.pack("h", len(where))
        buff += where

        # (4)排序
        order = req.order.strip()
        order = order.split(",") if order else []
        buff += struct.pack("h", len(order))
        for x in order:
            seg = x.strip().lower().split(" ")
            if not seg:
                print("[error] empty order")
                return []
            name = seg[0]
            if name not in self.fields:
                print(f"[error] 排序字段不存在：{name}")
                return []
            sort = 1 # 升序
            if len(seg) > 1:
                if seg[-1] == "desc":
                    sort = 2
                elif seg[-1] == "asc":
                    sort = 1
                else:
                    print("[error] 排序方式错误：{}".format(seg[-1]))
                    return []

            fid,ftype,fsize = self.fields[name]
            buff += struct.pack("h", fid)
            buff += struct.pack("b", sort)

        # (5)limit
        buff += struct.pack("ii", req.offset, req.size)

        # 发送buff
        resp = self.__send_and_recv(Operation.GET.value, buff)
        if self.__check_error(resp):
            return []

        # 响应
        count, = struct.unpack("i", resp[0:4])
        resp = resp[4:]
        ret = []
        for i in range(count):
            size, = struct.unpack("i", resp[0:4])
            ret.append(self.__output(resp[4:4+size], fout, req.fmt))
            resp = resp[4+size:]

        return ret


    def __output(self, buff:bytes, fout:list, fmt:str):
        data = []
        for fid,fname,ftype,fsize in fout:
            if ftype == Type.kchar.value:
                l, = struct.unpack("h", buff[0:2])
                v = buff[2:2+l].decode()
                buff = buff[2+l:]
            elif ftype == Type.kint1.value:
                v, = struct.unpack("b", buff[:1])
                buff = buff[1:]
            elif ftype == Type.kint2.value:
                v, = struct.unpack("h", buff[:2])
                buff = buff[2:]
            elif ftype == Type.kint4.value:
                v, = struct.unpack("i", buff[:4])
                buff = buff[4:]
            elif ftype == Type.kint8.value:
                v, = struct.unpack("q", buff[:8])
                buff = buff[8:]
            elif ftype == Type.kfloat.value:
                v, = struct.unpack("f", buff[:4])
                if math.isinf(v) or math.isnan(v):
                    v = 0
                else:
                    v = round(v, 2)
                buff = buff[4:]
            elif ftype == Type.kdouble.value:
                v, = struct.unpack("d", buff[:8])
                if math.isinf(v) or math.isnan(v):
                    v = 0
                else:
                    v = round(v, 2)
                buff = buff[8:]
            data.append(v)
        if fmt == "[]":
            return data
        else:
            t = {}
            i = 0
            for _,name,_,_ in fout:
                t[name] = data[i]
                i += 1
            return t


    def delete(self, req:Request) -> int:
        buff = b""

        # (2)关键字
        keys = req.keys.strip()
        keys = keys.split(",") if keys else []
        buff += struct.pack("h", len(keys))
        for x in keys:
            x = x.strip()
            if len(x) > 64:
                print(f"[error]key太长:{x}")
                return []
            buff += struct.pack("h", len(x))
            buff += x.encode()

        # (3)查询条件
        where = req.where.encode()
        buff += struct.pack("h", len(where))
        buff += where

        # 发送
        resp = self.__send_and_recv(Operation.DEL.value, buff)
        if self.__check_error(resp):
            return 0

        size, = struct.unpack("i", resp[0:4])
        return size

    
    def agg(self, req:Request) -> list:
        """
        聚合函数，count/sum/avg/max/min，只能指定单个字段
        """
        buff = b""

        fout = [] # 用于输出
        # (1)字段ID列表、名字列表
        aggs = []
        for func in req.aggs.replace(" ", "").split(","):
            if not func:
                continue
            func = func.lower()

            # func=count(1),  func=max(incr),  func=avg(close)
            if func[-1] != ')':
                print(f"[error] 聚合函数格式错误. {func}")
                return []
            seg = func[:-1].split("(")
            if len(seg) != 2:
                print(f"[error] 聚合函数格式错误. {func}")
                return []
            f,name = seg
            if f == "count":
                aggs.append([1,0])
                fid,ftype,fsize = 0,Type.kint4.value,0
            else:
                if name not in self.fields:
                    print(f"[error] 字段不存在: {func}")
                    return []
                fid,ftype,fsize = self.fields[name]
                if f == "sum":
                    aggs.append([2,fid])
                elif f == "avg":
                    aggs.append([3,fid])
                elif f == "max":
                    aggs.append([4,fid])
                elif f == "min":
                    aggs.append([5,fid])
                else:
                    print(f"[error] 函数不支持: {func}")
                    return []
                if ftype == Type.kchar.value:
                    if f == "sum" or f == "avg":
                        print(f"[error] 字符串类型字段不支持sum/avg：{func}")
                        return []
                else:
                    ftype = Type.kdouble.value

            fout.append([fid, func, ftype, fsize])

        if not aggs:
            print("[error] 没有聚合函数")
            return []

        buff += struct.pack("h", len(aggs))
        for aid,fid in aggs:
            buff += struct.pack("hh", aid,fid)

        # (3)查询条件
        where = req.where.encode()
        buff += struct.pack("h", len(where))
        buff += where

        # (4)group by
        fout2 = []
        fids = []
        for name in req.group.split(","):
            name = name.strip().lower()
            if not name:
                continue
            if name not in self.fields:
                print(f"[error] 字段不存在: {name}")
                return []
            fid,ftype,fsize = self.fields[name]
            fids.append(fid)
            fout2.append([fid, name, ftype, fsize])

        buff += struct.pack("h", len(fids))
        for x in fids:
            buff += struct.pack("h", x)

        # 发送buff
        resp = self.__send_and_recv(Operation.AGG.value, buff)
        if self.__check_error(resp):
            return []

        # 响应
        count, = struct.unpack("i", resp[0:4])
        resp = resp[4:]
        ret = []
        for i in range(count):
            size, = struct.unpack("i", resp[0:4])
            ret.append(self.__output(resp[4:4+size], fout2+fout, req.fmt))
            resp = resp[4+size:]

        return ret


    def select(self, sql:str, dict:bool=False) -> list:
        """
        读取数据
        @sql格式（没有from）: select ___ where ___ group by ___ order by ___ limit ___,___
        @dict：数据格式，True字典，False列表
        """
        sql = sql.lower()
        sql = sql.replace("\n", " ").replace("\t", " ").replace("\r", " ")
        while "  " in sql:
            sql = sql.replace("  ", " ")
        
        # 找出所有的关键字
        keypoints = []
        p = sql.find("select")
        if p != -1:
            keypoints.append([p, "select"])
        p = sql.find("where")
        if p != -1:
            keypoints.append([p, "where"])
        p = sql.find("group by")
        if p != -1:
            keypoints.append([p, "group by"])
        p = sql.find("order by")
        if p != -1:
            keypoints.append([p, "order by"])
        p = sql.find("limit")
        if p != -1:
            keypoints.append([p, "limit"])

        keypoints.append([len(sql), ""])
        keypoints.sort(key=lambda x:x[0])

        # 构造请求
        req = Request()
        req.fmt = "{}" if dict else "[]"

        group = False
        for i in range(len(keypoints)-1):
            p,name = keypoints[i]
            nextp,_ = keypoints[i+1]
            s = sql[p+len(name)+1:nextp]
            if name == "select":
                req.fields = s
                if "(" in req.fields:
                    req.aggs = req.fields
                    group = True
            elif name == "where":
                req.where = s
            elif name == "group by":
                req.group = s
            elif name == "order by":
                req.order = s
            elif name == "limit":
                if "," in s:
                    a,b = s.split(",")
                    req.offset = int(a.strip())
                    req.size = int(b.strip())
                else:
                    req.offset = 0
                    req.size = int(s.strip())

        if group:
            return self.agg(req)
        else:
            return self.get(req)



if __name__ == '__main__':
    c = Client()
    """
    print(c.conf())
    print(c.status())
    print(c.column())
    print(c.detail())
    print(c.dump())
    """
    data = [
        ["000001.SZ", {"name":"123", "code":"123"}],
        ["000001.SZ", {"name":"1234", "code":"124"}],
    ]
    #print(c.setmore(data))

    # 普通查询
    req = Request()
    req.fields = "code,name,incr,close,mv"
    #req.where = "incr>1 and name like '电力'"
    req.where = "incr>1 and name like 'ST'" 
    for x in c.get(req):
        print(x)
    """
    req.order = "incr desc"
    req.fmt = "{}"
    #print("delete", c.delete(req))
    req.fields = "*"
    req.keys = "600519.SH,688244.SH"
    req.fmt = "{}"
    for x in c.get(req):
        print(x)
    """

    '''
    req.aggs = "count(1),avg(close),max(close),min(close),max(code)"
    req.group = "type,code"
    for x in c.agg(req):
        print(x)
    '''
    sql = "select code,name,incr where incr>0 order by incr desc limit 10"
    sql = "select count(1),avg(close),max(close),min(close),max(code) where incr>0 group by type"
    for x in c.select(sql):
        print(x)

    #time.sleep(10)
