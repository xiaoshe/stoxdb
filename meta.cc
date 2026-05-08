#include "meta.h"

#include <string.h>
#include "base.h"


Meta::Meta() {
    port_ = 30634;
    dump_file_ = "db.dump";
    magic_number_ = 20251208;
    max_recv_size_ = 4194304;
    max_connections_ = 1024;
    idle_timeout_ = 120;
    transport_timeout_ = 30;
}
Meta::~Meta() {
    for (size_t i = 0; i < fields_.size(); i++) {
        Field& f = fields_[i];
        free_expression(f.formula);
    }
}

void print(int i, const Field& f) {
    printf("[%d][%s] type:[%s] offset:%d size:%d note:%s\n", i, f.name.c_str(), gtype[f.type], f.offset, f.size, f.note.c_str());
    print_expression(f.formula);
    if (f.decide.size() > 0) {
        printf("decide:");
        for (size_t i = 0; i < f.decide.size(); i++) {
            printf("%d ", f.decide[i]);
        }
        printf("\n");
    }
}

bool tofield(const string& s, Field *f) {
    //code|char(12)|股票代码
    //vol|float|成交量
    memset(f, 0, sizeof(Field));
    vector<string> seg;
    strsplit(s, '|', &seg);
    if ((int)seg.size() < 3) {
        printf("[error] field segment < 3\n");
        return false;
    }

    // name
    if (seg[0].size() > 32) {
        printf("[error] field length > 32. ===> %s\n", seg[0].c_str());
        return false;
    }
    f->name = seg[0];
    tolower(f->name);

    // type
    f->rtype = seg[1];
    const string& t = seg[1];
    if (t == "int1") {
        f->type = kint1;
        f->size = 1;
    } else if (t == "int2") {
        f->type = kint2;
        f->size = 2;
    } else if (t == "int4") {
        f->type = kint4;
        f->size = 4;
    } else if (t == "int8") {
        f->type = kint8;
        f->size = 8;
    } else if (t == "int") {
        f->type = kint4;
        f->size = 4;
    } else if (t == "float") {
        f->type = kfloat;
        f->size = 4;
    } else if (t == "double") {
        f->type = kdouble;
        f->size = 8;
    } else if (t.size() > 4 && strncmp(t.c_str(), "char", 4)==0) {
        f->type = kchar;
        int sz = digit(t.c_str()+4);
        if (sz <= 0 || sz >= 1000) {
            printf("[error] char type is too long (0<N<1000). ===> %s\n", t.c_str());
            return false;
        }
        // 字符串数据存储前面增加2个字节，表示字符串的实际长度(0 ~ 255)，避免int8_t越界，强制转化成uint8_t麻烦
        // 实际长度 <= sz(申请的空间大小)
        f->size = sz + 2;
    } else {
        printf("[error] not supported type. ===> %s\n", t.c_str());
        return false;
    }

    // note
    if (seg[2].size() > 256) {
        printf("[error] note length > 256. ===> %s\n", seg[2].c_str());
        return false;
    }
    f->note = seg[2];
    //print(*f);
    return true;
}


bool Meta::load_config() {
    // 读配置文件
    vector<string> conf;
    FILE *rp = fopen("db.conf", "r");
    char buf[4096];
    while (fgets(buf, 4096, rp)) {
        char *p = buf;
        string s;
        while (*p != 0 && *p != '#') {
            if (*p != ' ' && *p != '\n')
                s.push_back(*p);
            p++;
        }
        if (s.empty()) {
            continue;
        }
        conf.push_back(s);
        //printf("%s\n", s.c_str());
    }
    fclose(rp);

    // 端口号
    int p = scanconf(conf, "[server]");
    for (int i = p+1; i<(int)conf.size(); i++) {
        const string& s = conf[i];
        if (s[0] == '[') break;
        vector<string> seg;
        strsplit(s, '=', &seg);
        if (seg.size() != 2) continue;
        if (seg[0] == "port") port_ = digit(seg[1].c_str());
        if (seg[0] == "dump_file") dump_file_ = seg[1];
        if (seg[0] == "magic_number") magic_number_ = digit(seg[1].c_str());
        if (seg[0] == "max_recv_size") max_recv_size_ = digit(seg[1].c_str());
        if (seg[0] == "max_connections") max_connections_ = digit(seg[1].c_str());
        if (seg[0] == "idle_timeout") idle_timeout_ = digit(seg[1].c_str());
        if (seg[0] == "transport_timeout") transport_timeout_ = digit(seg[1].c_str());
    }

    // 字段
    p = scanconf(conf, "[field]");
    for (int i = p+1; i<(int)conf.size(); i++) {
        const string& s = conf[i];
        if (s[0] == '[') break;
        Field f;
        if (!tofield(s, &f)) {
            return false;
        }
        if (fields_.size() > 16000) {
            printf("[error] field number > 16000");
            return false;
        }
        int id = getid(f.name);
        if (id >= 0) {
            printf("[error] unique field. ===> %s\n", f.name.c_str());
            return false;
        }
        fields_.push_back(f);
        fname_id_[f.name] = int(fields_.size()) - 1;
    }
    // 偏移量
    int offset = 0;
    for (size_t i = 0; i < fields_.size(); i++) {
        fields_[i].offset = offset;
        offset += fields_[i].size;
    }
    record_size_ = offset;

    // 公式
    p = scanconf(conf, "[formula]");
    for (int i = p+1; i<(int)conf.size(); i++) {
        tolower(conf[i]);
        const string& s = conf[i];
        if (s[0] == '[') break;
        //printf("formula:%s\n", s.c_str());

        // 输入：amp=(high-low)/preclose*100
        // = 将字符串分成左右两部分
        vector<string> seg;
        strsplit(s, '=', &seg);
        if (seg.size() != 2) {
            printf("[error] formula error. ===> %s\n", s.c_str());
            return false;
        }
        unordered_map<string, int>::const_iterator it = fname_id_.find(seg[0]);
        if (it == fname_id_.end()) {
            // 所属字段未找到
            printf("[error] '%s' not found\n", seg[0].c_str());
            return false;
        }
        int id = it->second;
        Field& fd = fields_[id];

        if (fd.type == kchar) {
            printf("[error] char type cannot computed by formula. ===> %s\n", fd.name.c_str());
            return false;
        }

        // 加载表达式
        string errmsg;
        if (!load_expression(seg[1].c_str(), seg[1].size(), &fd.formula, &errmsg)) {
            printf("[error] %s\n", errmsg.c_str());
            return false;
        }
        // 检查表达式是否有效
        if (!check_expression(fd.formula, &errmsg)) {
            printf("[error] %s\n", errmsg.c_str());
            return false;
        }

        // 检查表达式中的字段是否存在
        for (int i = 0; i < fd.formula.size(); i++) {
            Node *x = fd.formula[i];
            if (x->type != kname) continue;
            unordered_map<string, int>::const_iterator it = fname_id_.find(x->vs);
            if (it == fname_id_.end()) {
                printf("[error] '%s' not found\n", x->vs.c_str());
                return false;
            }
            x->type = kid;
            x->id = it->second;
            fields_[it->second].decide.push_back(id); // 公式依赖关系
        }

        // 等号右边部分
        fd.rform = seg[1];
    }
    // 检查依赖关系是否存在环
    for (int i = 0; i < (int)fields_.size(); i++) {
        if (has_circle(i)) {
            printf("[error] formula has circle.\n");
            return false;
        }
    }

    return true;
}

bool Meta::has_circle(int fid) const {
    const Field & f = fields_[fid];

    vector<int> curr = f.decide;
    while (!curr.empty()) {
        vector<int> next;
        for (size_t i = 0; i < curr.size(); i++) {
            if (curr[i] == fid) return true;
            for (size_t j = 0; j < fields_[curr[i]].decide.size(); j++) {
                next .push_back(fields_[curr[i]].decide[j]);
            }
        }
        curr = next;
    }

    return false;
}

int Meta::getid(const string& name) const {
    unordered_map<string, int>::const_iterator it = fname_id_.find(name);
    if (it == fname_id_.end()) return -1;
    return it->second;
}

void Meta::info() const {
    printf("\n=== 配置信息 ===\n");
    printf("  字段个数 -------------------------------> %d\n", (int)fields_.size());
    printf("  单条记录大小 ---------------------------> %d (Bytes)\n", record_size_);
    printf("  监听端口(port) -------------------------> %d\n", port_);
    printf("  导出文件名(dump_file) ------------------> %s\n", dump_file_.c_str());
    printf("  魔法数字(magic_number) -----------------> %d\n", magic_number_);
    printf("  最大接收字节数(max_recv_size) ----------> %d (Bytes)\n", max_recv_size_);
    printf("  最大连接数(max_connections) ------------> %d\n", max_connections_);
    printf("  空闲连接超时时间(idle_timeout) ---------> %d (秒)\n", idle_timeout_);
    printf("  传输过程中超时时间(transport_timeout) --> %d (秒)\n", transport_timeout_);
    printf("\n");
}

void Meta::desc() const {
    printf("\n=== 字段信息 ===\n");
    printf("字段数量：%ld:\n", fields_.size());
    for (size_t i = 0; i < fields_.size(); i++) {
        print(i, fields_[i]);
    }
    printf("字段名 => 字段ID：\n");
    for (unordered_map<string, int>::const_iterator it = fname_id_.begin(); it != fname_id_.end(); it++) {
        printf("%32s -> %d\n", it->first.c_str(), it->second);
    }
}
