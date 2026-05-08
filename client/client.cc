#include "client.h"
#include "../base.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

Client::Client(const string& host, int port, int magic)
    : host_(host), port_(port), magic_(magic) {
    init_buffer(&buffer_, 8192);

    while (!Connect()) {
        printf("connect error. %s:%d\n", host_.c_str(), port_);
        sleep(1);
    }
    UpdateFields();
}

Client::~Client() {
    close(sockfd_);
    free_buffer(&buffer_);
}

bool Client::Connect() {
    sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_ < 0) {
        perror("socket");
        return false;
    }
    
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port_);
    inet_pton(AF_INET, host_.c_str(), &serv_addr.sin_addr);
    
    // 阻塞式连接
    return connect(sockfd_, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) != -1;
}


bool Client::Send() {
    int size = 32*1024;
    int offset = 0;
    int total = buffer_.size;
    while (offset < total) {
        int left = total - offset;
        if (left > size) left = size;
        int sent = (int)send(sockfd_, buffer_.data + offset, left, 0);
        if (sent <= 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 缓冲区满，等待
                printf("缓冲区满，等待\n");
                continue;
            }
            perror("send packet");
            return false;
        }
        offset += sent;
    }
    printf("发送成功。bytes:%d\n", total);
    return true;
}

bool Client::Recv() {
    // 接收状态字节
    int n = (int)recv(sockfd_, buffer_.data, 4, 0);
    if (n != 4) {
        if (n == 0) {
            printf("服务器关闭连接\n");
            return false;
        }
        perror("recv status");
        return false;
    }

    int size = *(int*)buffer_.data;
    buffer_.offset = 4; // 重置其实位置
    buffer_.size = size + 4;
    if (!expand_buffer(&buffer_, size)) {
        return false;
    }

    int total_recv = 0;
    while (total_recv < size) {
        n = (int)recv(sockfd_, buffer_.data + 4 + total_recv, size - total_recv, 0);
        if (n <= 0) {
            perror("recv message");
            return false;
        }
        total_recv += n;
    }
    size_ = size - 4; // 减去status
    status_ = *(int*)(buffer_.data+4);
    data_ = buffer_.data + 8;
    printf("接收成功. bytes:%d\n", buffer_.size);
    return true;
}

void Client::SetHeader(int size, int opt) {
    buffer_.size = 12 + size;
    memcpy(buffer_.data, &magic_, sizeof(int));
    memcpy(buffer_.data+4, &size, sizeof(int));
    memcpy(buffer_.data+8, &opt, sizeof(int));
}

bool Client::Error() const {
    if (status_ == OK) {
        return false;
    } else {
        string errmsg;
        getchar2(data_, &errmsg);
        printf("error:%s\n", errmsg.c_str());
        return true;
    }
}

bool Client::Conf() {
    SetHeader(0, OPT_CONF);
    if (Send() && Recv()) {
        if (Error()) {
            return false;
        }
        char *p = data_;
        int port, magic, max_recv_size, max_connections, record_size, field_count, data_count;
        string dumpfile;
        p += getint4(p, &port);
        printf("端口:%d\n", port);
        p += getchar1(p, &dumpfile);
        printf("dumpfile:%s\n", dumpfile.c_str());
        p += getint4(p, &magic);
        printf("魔法数字:%d\n", magic);
        p += getint4(p, &max_recv_size);
        printf("最大接收字节数:%d\n", max_recv_size);
        p += getint4(p, &max_connections);
        printf("最大链接数:%d\n", max_connections);
        p += getint4(p, &record_size);
        printf("单条记录大小:%d\n", record_size);
        p += getint4(p, &field_count);
        printf("字段数量:%d\n", field_count);
        p += getint4(p, &data_count);
        printf("数据条数:%d\n", data_count);
        return true;
    }
    return false;
}

bool Client::Status() {
    SetHeader(0, OPT_STATUS);
    if (Send() && Recv()) {
        if (Error()) {
            return false;
        }
        char *p = data_;
        int64_t count, total, request, error, response;
        int64_t rbytes, sbytes;
        p += getint8(p, &count);
        printf("当前连接数:%ld\n", count);
        p += getint8(p, &total);
        if (total < 10000) {
            printf("连接总次数:%ld\n", total);
        } else {
            printf("连接总次数:%.1fw\n", total/10000.0);
        }
        p += getint8(p, &request);
        if (request < 10000) {
            printf("请求总次数:%ld\n", request);
        } else {
            printf("请求总次数:%.1fw\n", request/10000.0);
        }
        p += getint8(p, &error);
        if (error < 10000) {
            printf("错误总次数:%ld\n", error);
        } else {
            printf("错误总次数:%.1fw\n", error/10000.0);
        }
        p += getint8(p, &response);
        if (response < 10000) {
            printf("响应总次数:%ld\n", response);
        } else {
            printf("响应总次数:%.1fw\n", response/10000.0);
        }
        p += getint8(p, &rbytes);
        if (rbytes < 1024) {
            printf("接收字节总数:%ld\n", rbytes);
        } else if (rbytes < 1024*1024) {
            printf("接收字节总数:%.1fK\n", rbytes/1024.0);
        } else if (rbytes < 1024*1024*1024) {
            printf("接收字节总数:%.1fM\n", rbytes/1024.0/1024.0);
        } else {
            printf("接收字节总数:%.1fG\n", rbytes/1024.0/1024.0/1024.0);
        }
        p += getint8(p, &sbytes);
        if (sbytes < 1024) {
            printf("发送字节总数:%ld\n", sbytes);
        } else if (sbytes < 1024*1024) {
            printf("发送字节总数:%.1fK\n", sbytes/1024.0);
        } else if (sbytes < 1024*1024*1024) {
            printf("发送字节总数:%.1fM\n", sbytes/1024.0/1024.0);
        } else {
            printf("发送字节总数:%.1fG\n", sbytes/1024.0/1024.0/1024.0);
        }

        return true;
    }
    return false;
}

bool Client::Column(vector<Field> *fields) {
    fields->clear();
    SetHeader(0, OPT_COLUMN);
    if (Send() && Recv()) {
        if (Error()) {
            return false;
        }
        char *p = data_;
        while (p - data_ < size_) {
            Field f;
            int8_t type;
            p += getint1(p, &type);
            int16_t size;
            p += getint2(p, &size);
            p += getchar1(p, &f.name);
            f.type = (Type)type;
            f.size = size;
            fields->push_back(f);
        }
        return true;
    }
    return false;
}


bool Client::UpdateFields() {
    if (!Column(&fields_)) return false;

    fname_map_.clear();
    for (int i = 0; i < (int)fields_.size(); i++) {
        const Field& f = fields_[i];
        Simple sim;
        sim.id = i;
        sim.type = f.type;
        sim.sz = f.size;
        fname_map_[f.name] = sim;
    }
    
    for(map<string, Simple>::iterator it = fname_map_.begin(); it!=fname_map_.end(); it++) {
        printf("%32s -> {id:%d, type:%s, sz:%d}\n", it->first.c_str(), it->second.id, gtype[it->second.type], it->second.sz);
    }

    return true;
}

bool Client::Detail(vector<Field> *fields) {
    fields->clear();
    SetHeader(0, OPT_DETAIL);
    if (Send() && Recv()) {
        if (Error()) {
            return false;
        }
        char *p = data_;
        while (p - data_ < size_) {
            Field f;
            p += getchar1(p, &f.name);
            p += getchar1(p, &f.rtype);
            p += getchar2(p, &f.note);
            p += getchar2(p, &f.rform);
            fields->push_back(f);
        }
        return true;
    }
    return false;
}

int Client::Dump() {
    SetHeader(0, OPT_DUMP);
    if (Send() && Recv()) {
        if (Error()) {
            return -1;
        }
        return *(int*)(data_);
    }
    return -1;
}

Simple Client::getid(const string& name) const {
    map<string, Simple>::const_iterator it = fname_map_.find(name);
    if (it == fname_map_.end()) {
        printf("字段不存在. %s\n", name.c_str());
        Simple sim;
        sim.id = -1;
        sim.type = 0;
        return sim;
    } else {
        return it->second;
    }
}

void Client::begin() {
    // [头信息][数据1][数据2]...
    // [数据1] = [size][key][字段1][字段2][字段3]...
    buffer_.offset = 12;
}

void Client::end() {
    // 单条大小
    buffer_.size = buffer_.offset;
    int size = buffer_.size - 12;
    SetHeader(size, OPT_SET);
    if (Send() && Recv()) {
        if (Error()) {
            printf("error\n");
            return;
        }
        size = *(int*)data_;
        printf("receive bytes:%d update:%d\n", size_, size);
    } else {
        printf("error\n");
    }
}

void Client::setkey(const string& k) {
    if (k.size() > 64) {
        printf("[error]key太长:%s\n", k.c_str());
        return;
    }
    int size = 2 + (int)k.length();
    // key前面有一个4字节整数，表示本条数据的长度
    expand_buffer(&buffer_, 4+size);
    begin_ = buffer_.data + buffer_.offset; // 固定位置，存储单条数据大小
    buffer_.offset += 4;
    buffer_.offset += setchar2(buffer_.data + buffer_.offset, k);
    memcpy(begin_, &size, 4);
}

void Client::setvalue(const string& name, const string& v) {
    Simple sim = getid(name);
    if (sim.type != kchar) {
        printf("[error]类型错误. %s\n", name.c_str());
        return;
    }
    if ((int)v.size() > (int)sim.sz) {
        printf("[error]数据太长. %d > %d\n", (int)name.size(), (int)sim.sz);
        return;
    }

    int size = 4 + (int)v.length();
    expand_buffer(&buffer_, size);

    buffer_.offset += setint2(buffer_.data + buffer_.offset, (int16_t)sim.id);
    buffer_.offset += setchar2(buffer_.data + buffer_.offset, v);

    int sz = *(int*)begin_;
    sz += size;
    //printf("id=%d sz=%d\n", sim.id, sz);
    memcpy(begin_, &sz, 4);
}

void Client::setvalue(const string& name, double v) {
    Simple sim = getid(name);
    if (sim.type != kfloat && sim.type != kdouble) {
        printf("[error]类型错误. %s\n", name.c_str());
        return;
    }
    int size = 2 + sim.sz;
    expand_buffer(&buffer_, size);

    buffer_.offset += setint2(buffer_.data + buffer_.offset, (int16_t)sim.id);
    if (sim.type == kfloat) {
        buffer_.offset += setfloat(buffer_.data + buffer_.offset, (float)v);
    } else {
        buffer_.offset += setfloat(buffer_.data + buffer_.offset, v);
    }

    int sz = *(int*)begin_;
    sz += size;
    memcpy(begin_, &sz, 4);
}

void Client::setvalue(const string& name, int64_t v) {
    Simple sim = getid(name);
    if (sim.type != kint1 &&
        sim.type != kint2 &&
        sim.type != kint4 &&
        sim.type != kint8) {
        printf("[error]类型错误. %s\n", name.c_str());
        return;
    }
    int size = 2 + sim.sz;
    expand_buffer(&buffer_, size);

    buffer_.offset += setint2(buffer_.data + buffer_.offset, (int16_t)sim.id);
    if (sim.type == kint1) buffer_.offset += setint1(buffer_.data + buffer_.offset, (int8_t)v);
    if (sim.type == kint2) buffer_.offset += setint2(buffer_.data + buffer_.offset, (int16_t)v);
    if (sim.type == kint4) buffer_.offset += setint4(buffer_.data + buffer_.offset, (int32_t)v);
    if (sim.type == kint8) buffer_.offset += setint8(buffer_.data + buffer_.offset, (int64_t)v);

    int sz = *(int*)begin_;
    sz += size;
    memcpy(begin_, &sz, 4);
}

void Client::setvalue(const string& name, int32_t v) {
    setvalue(name, (int64_t)v);
}


char *Client::output(const vector<int16_t>& fids, const vector<string>& fnames, char *p) {
    for (int j = 0; j < fids.size(); j++) {
        printf("  -  %s:", fnames[j].c_str());
        const Field& f = fields_[fids[j]];
        if (f.type == kchar) {
            string v;
            p += getchar2(p, &v);
            printf("%s\n", v.c_str());
        } else if (f.type == kint1) {
            int8_t v = *(int8_t*)p; p+=1;
            printf("%d\n", v);
        } else if (f.type == kint2) {
            int16_t v = *(int16_t*)p; p+=2;
            printf("%d\n", v);
        } else if (f.type == kint4) {
            int32_t v = *(int32_t*)p; p+=4;
            printf("%d\n", v);
        } else if (f.type == kint8) {
            int64_t v = *(int64_t*)p; p+=8;
            printf("%ld\n", v);
        } else if (f.type == kfloat) {
            float v = *(float*)p; p+=4;
            printf("%.2f\n", v);
        } else if (f.type == kdouble) {
            double v = *(double*)p; p+=8;
            printf("%.2f\n", v);
        }
    }
    return p;
}

bool parse_order(const string& order, vector<string> *out) {
    out->clear();
    vector<string> segs;
    strsplit(order, ',', &segs);
    for (size_t i = 0; i < segs.size(); i++) {
        trim(segs[i]);
        tolower(segs[i]);
        string t("1"); // 1正向，2反向
        const char *p = segs[i].c_str();
        while (*p != 0) {
            if (*p == ' ') {
                break;
            }
            t.push_back(*p);
            p++;
        }
        while (*p == ' ') p++;
        if (*p != 0) {
            string b(p);
            if (b == "desc") {
                t[0] = '2';
            } else if (b == "asc") {
            } else {
                printf("[error] 排序方式错误:%s\n", b.c_str());
                return false;
            }
        }
        out->push_back(t);
    }
    return true;
}

bool Client::get(const Request& req) {
    // 构建发送buffer_t
    char *start = buffer_.data + 12;
    char *p = start;


    // (1)字段
    vector<string> fnames;
    vector<int16_t> fids; // 字段id的集合，发送字段id即可
    if (req.fields.size() == 1 && req.fields[0] == '*') {
        for (size_t i = 0; i < fields_.size(); i++) {
            fids.push_back(i);
            fnames.push_back(fields_[i].name);
        }
    } else {
        strsplit(req.fields, ',', &fnames);
        for (size_t i = 0; i < fnames.size(); i++) {
            trim(fnames[i]);
            tolower(fnames[i]);
            Simple sim = getid(fnames[i]);
            if (sim.id == -1) {
                printf("[error]字段不存在:%s\n", fnames[i].c_str());
                return false;
            }
            fids.push_back(sim.id);
        }
    }

    int sz = (int)fids.size();
    if (sz == 0) {
        printf("[error]无字段\n");
        return false;
    }
    p += setint2(p, (int16_t)sz);
    for (int i = 0; i < sz; i++) {
        p += setint2(p, fids[i]);
    }
    

    // (2)关键字
    vector<string> segs;
    strsplit(req.keys, ',', &segs);
    for (size_t i = 0; i < segs.size(); i++) {
        trim(segs[i]);
        if (segs[i].size() > 64) {
            printf("[error]key太长：%s\n", segs[i].c_str());
            return false;
        }
    }
    sz = (int)segs.size();
    p += setint2(p, (int16_t)sz);
    for (int i = 0; i < sz; i++) {
        p += setchar2(p, segs[i]);
    }

    // (3)查询条件，字符串
    p += setchar2(p, req.where);


    // (4)排序，将排序字段字节化
    //p += setchar2(p, req.order);
    vector<string> order;
    if (!parse_order(req.order, &order)) {
        return false;
    }
    p += setint2(p, (int16_t)order.size());
    for (size_t i = 0; i < order.size(); i++) {
        const char *f = order[i].c_str() + 1;
        Simple sim = getid(f);
        if (sim.id == -1) {
            printf("[error]字段不存在:%s\n", f);
            return false;
        }
        int8_t t = order[i][0] == '1' ? 1 : 2;
        p += setint2(p, (int16_t)sim.id);
        p += setint1(p, (int8_t)t);
    }

    // (5)limit
    p += setint4(p, req.offset);
    p += setint4(p, req.size);

    // 发送 & 接收
    SetHeader(p-start, OPT_GET);
    if (Send() && Recv()) {
        if (Error()) {
            return false;
        }
        int count = *(int*)data_;
        char *p = data_+4;
        printf("size:%d status:%d count:%d\n", size_, status_, count);
        for (int i = 0; i < count; i++) {
            int bytes = *(int*)p; p+=4;
            printf("Data[%d][bytes=%d]:\n", i+1, bytes);
            p = output(fids, fnames, p);
        }
        return true;
    }
    return false;
}

