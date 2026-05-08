#include "record.h"

#include <unistd.h>
#include <string.h>
#include <set>
#include <map>
#include <unordered_map>
#include <algorithm>
#include "base.h"

char *create(int size) {
    char *r = (char *)malloc(size);
    if (r == NULL) {
        printf("[error] 内存不足. size=%d\n", size);
        exit(0);
    }
    memset(r, 0, size);
    return r;
}

void set_send_head(buffer_t *send, int data_size) {
    // [数据长度4][数据体]
    // 字节总长度 = 4字节 + length(数据体)
    // [data_size][长度为data_size的数据体]
    // data_size:4字节，代表数据长度，发送长度=data_size+4
    send->size = data_size + 4;
    setint4(send->data, data_size);
}

void set_header(buffer_t *send, int data_size, int status) {
    // [int][int][数据体]
    send->size = data_size + 8;
    setint4(send->data, data_size+4);
    setint4(send->data+4, status);
}


void return_error(buffer_t *send, const string& msg) {
    printf("errmsg:%s\n", msg.c_str());
    // [size][flag][msg]
    int flag = ERROR;
    memcpy(send->data + 4, &flag, 4);
    setchar2(send->data+8, msg);
    int size = 6 + (int)msg.length();
    memcpy(send->data, &size, 4);
    send->size = size + 4;
}


Record::Record(Meta *m) : meta_(m) {
    load();
    put_field_to_buffer();
}

Record::~Record() {
    free(field_buffer_);
    free_logical(lg_);
}


bool Record::setone(const string& key, const char *byte, int size) {
    if (key.size() > 64) {
        //printf("key size>64. %s\n", key.c_str());
        return false;
    }
    char *value = NULL; // map中的value
    unordered_map<string, char *>::iterator it = data_.find(key);
    if (it == data_.end()) {
        value = create(meta_->record_size_);
        data_[key] = value;
    } else {
        value = it->second;
    }

    // byte构成格式：[id][value]
    std::set<int> decide;
    const char *p = byte;
    while (p - byte < size) {
        int16_t id = *(int16_t*)p; p += 2;
        if (id < 0 || id > meta_->fields_.size()) {
            printf("id error. id=%d\n", id);
            return false;
        }
        const Field& f = meta_->fields_[id];
        if (f.type == kchar) {
            int16_t l = *(int16_t *)p;
            int sz = l + 2;
            memcpy(value + f.offset, p, sz);
            p += sz;
        } else {
            memcpy(value + f.offset, p, f.size);
            p += f.size;
        }

        for (size_t i = 0; i < f.decide.size(); i++) {
            decide.insert(f.decide[i]);
        }
    }

    // 计算公式字段
    //printf("decide %ld\n", decide.size());
    while (!decide.empty()) {
        std::set<int> next;
        for (std::set<int>::iterator it = decide.begin(); it != decide.end(); it++) {
            int id = *it;
            const Field& f = meta_->fields_[id];
            // 根据公式计算字段id的值
            double d = formula_compute(value, f.formula);
            //printf("compute d=%.2f\n", d);
            // 写入
            if (f.type == kint1) {
                int8_t t = (int8_t)d;
                memcpy(value+f.offset, &t, f.size);
            } else if (f.type == kint2) {
                int16_t t = (int16_t)d;
                memcpy(value+f.offset, &t, f.size);
            } else if (f.type == kint4) {
                int32_t t = (int32_t)d;
                memcpy(value+f.offset, &t, f.size);
            } else if (f.type == kint8) {
                int64_t t = (int64_t)d;
                memcpy(value+f.offset, &t, f.size);
            } else if (f.type == kfloat) {
                float t = (float)d;
                memcpy(value+f.offset, &t, f.size);
            } else if (f.type == kdouble) {
                memcpy(value+f.offset, &d, f.size);
            }

            for (size_t j = 0; j < f.decide.size(); j++) {
                next.insert(f.decide[j]);
            }
        }
        decide = next;
    }
    return true;
}


double Record::formula_compute(const char *value, const Expression& ex) {
    computer_.init();
    for (int i = 0; i < ex.size(); i++) {
        Node *x = ex[i];
        if (x->type == kleft || x->type == kright || is_four_type(x->type)) {
            // ( + - * / % )
            computer_.push_opt(x->type);
        } else if (is_digit_type(x->type)) {
            computer_.push_number(x->vd);
        } else {
            // 根据依赖的ID读取字段
            const Field& f = meta_->fields_[x->id];
            if (f.type == kint1) {
                int8_t a = *(int8_t*)(value + f.offset);
                computer_.push_number((double)a);
            } else if (f.type == kint2) {
                int16_t a = *(int16_t*)(value + f.offset);
                computer_.push_number((double)a);
            } else if (f.type == kint4) {
                int32_t a = *(int32_t*)(value + f.offset);
                computer_.push_number((double)a);
            } else if (f.type == kint8) {
                int64_t a = *(int64_t*)(value + f.offset);
                computer_.push_number((double)a);
            } else if (f.type == kfloat) {
                float a = *(float*)(value + f.offset);
                computer_.push_number((double)a);
            } else if (f.type == kdouble) {
                double a = *(double*)(value + f.offset);
                computer_.push_number((double)a);
            }
        }
    }
    return computer_.result();
}

void Record::data() const {
    for (unordered_map<string, char *>::const_iterator it = data_.begin(); it != data_.end(); it++) {
        printf("\n**key:%s\n", it->first.c_str());
        printf("**value:\n");

        char *p = it->second;
        for (size_t i = 0; i < meta_->fields_.size(); i++) {
            const Field& f = meta_->fields_[i];
                printf("    %12s = ", f.name.c_str());
                if (f.type == kint1) {
                    printf("%d", *(int8_t*)(p+f.offset));
                } else if (f.type == kint2) {
                    printf("%d", *(int16_t*)(p+f.offset));
                } else if (f.type == kint4) {
                    printf("%d", *(int32_t*)(p+f.offset));
                } else if (f.type == kint8) {
                    printf("%ld", *(int64_t*)(p+f.offset));
                } else if (f.type == kfloat) {
                    printf("%.2f", *(float*)(p+f.offset));
                } else if (f.type == kchar) {
                    int16_t l = *(int16_t*)(p+f.offset);
                    string s(p+f.offset+2, l);
                    printf("%s", s.c_str());
                }
                printf("\n");
        }
    }
}

int Record::size() const {
    return data_.size();
}


bool Record::opt_test(buffer_t *recv, buffer_t *send) const {
    // 数据原路返回
    header_t *header = (header_t *)recv->data;
    printf("recv header: magic:%d size:%d opt:%d\n", header->magic, header->size, header->opt);
    if (!expand_buffer(send, recv->capacity - send->capacity)) {
        return false;
    }
    send->offset = 0;
    send->size = recv->size;
    memcpy(send->data, recv->data, recv->size);
    return true;
}

bool Record::opt_conf(buffer_t *recv, buffer_t *send) const {
    char *start = send->data + 8;
    char *p = start;
    p += setint4(p, meta_->port_);          // 端口
    p += setchar1(p, meta_->dump_file_);    // dump文件
    p += setint4(p, meta_->magic_number_);  // 魔术数字
    p += setint4(p, meta_->max_recv_size_); // 最大接收字节数
    p += setint4(p, meta_->max_connections_); // 最大连接数
    p += setint4(p, meta_->record_size_);   // 每条记录大小
    p += setint4(p, (int)meta_->fields_.size()); // 字段数量
    p += setint4(p, (int)data_.size());    // 记录数量 

    set_header(send, p-start, OK);
    return true;
}

bool Record::opt_column(buffer_t *recv, buffer_t *send) const {
    // 向send中填充数据
    char *start = send->data + 8;
    if (!expand_buffer(send, field_buffer_size_)) return false;
    memcpy(start, field_buffer_, field_buffer_size_);
    set_header(send, field_buffer_size_, OK);
    return true;
}

bool Record::opt_detail(buffer_t *recv, buffer_t *send) const {
    char *start = send->data + 8;
    char *p = start;
    for (size_t i = 0; i < meta_->fields_.size(); i++) {
        const Field& f = meta_->fields_[i];
        // [name][type][note][formula]...
        int sz = (int)f.name.length() + (int)f.note.length() + (int)f.rtype.length() + (int)f.rform.length() + 6;
        if (!expand_buffer(send, sz)) return false;
        p += setchar1(p, f.name);
        p += setchar1(p, f.rtype);
        p += setchar2(p, f.note);
        p += setchar2(p, f.rform);
    }

    set_header(send, p - start, OK);
    return true;
}

int Record::dump() const {
    int count = 0;
    FILE *wp = fopen(meta_->dump_file_.c_str(), "wb");
    if (wp != NULL) {
        for (unordered_map<string, char *>::const_iterator it = data_.begin(); it != data_.end(); it++) {
            // key + value
            // key: 2字节+key
            // value: 4字节+value
            int16_t ksize = (int16_t)it->first.size();
            fwrite(&ksize, 2, 1, wp);
            fwrite(it->first.c_str(), ksize, 1, wp);

            int vsize = meta_->record_size_;
            fwrite(&vsize, 4, 1, wp);
            fwrite(it->second, vsize, 1, wp);

            count++;
        }
        fclose(wp);
    } else {
        count = -1;
    }
    printf("[info]备份数据完成。size=%d\n", count);
    return count;
}

bool Record::opt_dump(buffer_t *recv, buffer_t *send) const {
    int count = dump();
    setint4(send->data+8, count);
    set_header(send, 4, OK);
    return true;
}

bool Record::opt_set(buffer_t *recv, buffer_t *send) {
    // [头信息][单条数据大小][key][fields]
    //   单条数据大小 = key + fields
    header_t *header = (header_t*)recv->data;
    //printf("recv. magic:%d size:%d opt:%d\n", header->magic, header->size, header->opt);
    char *start = recv->data + HEADER_SIZE;
    char *p = start;
    int ok = 0;
    while (p - start < header->size) {
        int size = *(int*)p; p += 4;

        string key;
        p += getchar2(p, &key);

        size = size - 2 - (int)key.length();
        //printf("key:%s size=%d\n", key.c_str(), size);
        setone(key, p, size) && ok++;
        p += size;
    }

    setint4(send->data+8, ok);
    //printf("write %d\n", ok);
    set_header(send, 4, OK);
    return true;
}


// ******** opt_get ******** //
bool Record::parse_get_request(char *buff, int size) {
    char *start = buff + HEADER_SIZE;
    char *p = start;
    if (p - buff > size) return false;
    request_.fields.clear();
    request_.keys.clear();
    request_.where.clear();
    request_.order.clear();
    request_.offset = 0;
    request_.size = -1;

    // fields
    int16_t sz = *(int16_t*)p; p += 2; // 字段个数
    if (sz == 0) return false;
    for (int i = 0; i < sz; i++) {
        int16_t id = *(int16_t*)p; p += 2;
        if (id < -1 || id > (int16_t)meta_->fields_.size()) {
            return false;
        }
        request_.fields.push_back(id);
    }
    if (request_.fields[0] == -1) {
        // * 全部字段
        request_.fields.clear();
        for (size_t i = 0; i < meta_->fields_.size(); i++) {
            request_.fields.push_back((int16_t)i);
        }
    }

    // keys
    sz = *(int16_t*)p; p += 2; // keys个数
    if (sz > 0) {
        for (int i = 0; i < sz; i++) {
            string k;
            p += getchar2(p, &k);
            request_.keys.push_back(k);
        }
    }

    // where
    sz = *(int16_t*)p; p += 2;
    request_.where.assign(p, sz);
    p += sz;
    if (p - buff > size) return false;

    // order
    sz = *(int16_t*)p; p += 2; // 排序字段个数
    if (sz > 0) {
        for (int i = 0; i < sz; i++) {
            order_t od;
            od.id = *(int16_t*)p; p += 2;
            od.type = *(int8_t*)p; p += 1;
            request_.order.push_back(od);
        }
    }
    if (p - buff > size) return false;

    // limit
    request_.offset = *(int*)p; p += 4;
    request_.size = *(int*)p; p += 4;
    if (p - buff != size) return false;

    return true;
}

Type expression_type(Expression& ex, const Meta *meta, string *errmsg) {
    // 表达式中的字段名字改为字段ID
    bool has_double = false;
    bool has_int    = false;
    bool has_string = false;

    for (size_t i = 0; i < ex.size(); i++) {
        Node *x = ex[i];
        Type type = x->type;
        if (x->type == kname) {
            int id = meta->getid(x->vs);
            if (id < 0) {
                errmsg->assign(string("cannot find field:") + x->vs);
                return kerror;
            }
            x->id = id;
            type = meta->fields_[id].type;
        }
        if (type == kchar) {
            has_string = true;
        } else if (type == kfloat || type == kdouble) {
            has_double = true;
        } else if (type >= kint1 && type <= kint8) {
            has_int = true;
        }
        //printf("type:%s\n", gtype[type]);
    }

    if (has_string) {
        if (has_int || has_double) {
            errmsg->assign("both number and string in expression");
            return kerror;
        } else {
            return kchar;
        }
    } else {
        if (has_int) {
            if (has_double) return kdouble;
            else return kint8;
        } else {
            if (has_double) return kdouble;
            else {
                errmsg->assign("both number and string not in expression");
                return kerror;
            }
        }
    }
}

bool change_fname_to_fid(const Logical& lg, const Meta *meta, string *errmsg) {
    // 将字段名改成字段id
    // （1）检查字段是否存在
    // （2）检查表达式两边类型是否相同（数字、字符串）
    // （3）检查like字段类型是否是字符串
    for (size_t i = 0; i < lg.size(); i++) {
        LogicalNode *x = lg[i];
        if (is_compare_type(x->type) || is_like_type(x->type)) {
            Type left = expression_type(x->left, meta, errmsg);
            if (left == kerror) {
                return false;
            }
            Type right = expression_type(x->right, meta, errmsg);
            if (right == kerror) {
                return false;
            }
            if (is_like_type(x->type)) {
                if (left != kchar || right != kchar) {
                    errmsg->assign("like must be string field");
                    return false;
                }
                // 必须只能有一个字段
                if (x->left.size() != 1 || x->right.size() != 1) {
                    errmsg->assign("like must be one field");
                    return false;
                }
            }
            if ((left == kchar && right != kchar) || (left != kchar && right == kchar)) {
                errmsg->assign("diffrent type of expression");
                return false;
            }
            if (left == kchar && right == kchar) {
                // 必须只能有一个字段
                if (x->left.size() != 1 || x->right.size() != 1) {
                    errmsg->assign("string must be one field");
                    return false;
                }
            }
            // 表达式的类型，计算等号时，浮点数稍微有差异
            if (left == kchar) {
                x->extype = kchar;
            } else {
                if (left == kdouble || right == kdouble) {
                    x->extype = kdouble;
                } else {
                    x->extype = kint8;
                }
            }
        }
    }
    return true;
}

void Record::put_stack(char c) {
    char ret = c;
    while (lg_stack_size_ > 0) {
        char top = lg_stack_[lg_stack_size_-1];
        if (top == 'A') {
            char l = lg_stack_[lg_stack_size_-2];
            lg_stack_size_ -= 2;
            if (l == 'T' && ret == 'T') {
                ret = 'T';
            } else {
                ret = 'F';
            }
        } else if (top == 'O') {
            char l = lg_stack_[lg_stack_size_-2];
            lg_stack_size_ -= 2;
            if (l == 'T' || ret == 'T') {
                ret = 'T';
            } else {
                ret = 'F';
            }
        } else {
            break;
        }
    }
    lg_stack_[lg_stack_size_++] = ret;
}

bool Record::compute_logical(const char *value) {
    /*
     * 根据条件过滤
     *  将逻辑表达式转化为序列，AND或OR优先级不确定，例如
     *  1: T A T A F
     *  2: (T O F) A (T)
     *  3: (T) A (F) O (T A F)
     */
    lg_stack_size_ = 0; // 从0开始

    for (size_t i = 0; i < lg_.size(); i++) {
        LogicalNode *x = lg_[i];
        if (x->type == kleft) {
            // 入栈
            lg_stack_[lg_stack_size_++] = '(';
        } else if (x->type == kand) {
            // 入栈
            lg_stack_[lg_stack_size_++] = 'A';
        } else if (x->type == kor) {
            // 入栈
            lg_stack_[lg_stack_size_++] = 'O';
        } else if (x->type == kright) {
            // 出栈
            char c = lg_stack_[--lg_stack_size_];
            lg_stack_size_--;
            put_stack(c);
        } else if (is_like_type(x->type)) {
            const Field& f = meta_->fields_[x->left[0]->id];
            string& v1 = x->right[0]->vs;
            int16_t l = *(int16_t*)(value + f.offset);
            string v(value+f.offset+2, l);
            // v:数值，v1:用户输入，^ST$
            char ret = 'F';
            if (x->like_start) {
                if (x->like_end) {
                    ret = (v == v1) ? 'T' : 'F';
                } else {
                    ret = startwith(v, v1) ? 'T' : 'F';
                }
            } else {
                if (x->like_end) {
                    ret = endwith(v, v1) ? 'T' : 'F';
                } else {
                    ret = contain(v, v1) ? 'T' : 'F';
                }
            }
            //printf("value:%s %s start:%d end:%d ret:%c\n", v1.c_str(), v.c_str(), x->like_start, x->like_end, ret);
            if (x->type == knlike) {
                ret = (ret == 'T') ? 'F' : 'T';
            }
            put_stack(ret);
        } else if (is_compare_type(x->type)) {
            char ret;
            if (x->extype == kchar) {
                // 字符串表达式：左侧字段，右侧字符串数值
                const Field& f = meta_->fields_[x->left[0]->id];
                const string& v1 = x->right[0]->vs;
                int16_t l = *(int16_t*)(value + f.offset);
                string v(value+f.offset+2, l);
                //printf("value:%s %s\n", v1.c_str(), v.c_str());
                if (x->type == keq) {
                    ret = (v == v1) ? 'T' : 'F';
                } else if (x->type == kgt) {
                    ret = (v > v1) ? 'T' : 'F';
                } else if (x->type == kge) {
                    ret = (v >= v1) ? 'T' : 'F';
                } else if (x->type == klt) {
                    ret = (v < v1) ? 'T' : 'F';
                } else if (x->type == kle) {
                    ret = (v <= v1) ? 'T' : 'F';
                } else if (x->type == kne) {
                    ret = (v != v1) ? 'T' : 'F';
                } else {
                    ret = 'F';
                }
            } else {
                // 非字符串
                double left = formula_compute(value, x->left);
                double right = formula_compute(value, x->right);
                if (x->type == keq) {
                    if (x->extype == kint8) {
                        ret = ((int64_t)left == (int64_t)right) ? 'T' : 'F';
                    } else {
                        ret = (left == right) ? 'T' : 'F';
                    }
                } else if (x->type == kgt) {
                    if (x->extype == kint8) {
                        ret = ((int64_t)left > (int64_t)right) ? 'T' : 'F';
                    } else {
                        ret = (left > right) ? 'T' : 'F';
                    }
                } else if (x->type == kge) {
                    if (x->extype == kint8) {
                        ret = ((int64_t)left >= (int64_t)right) ? 'T' : 'F';
                    } else {
                        ret = (left >= right) ? 'T' : 'F';
                    }
                } else if (x->type == klt) {
                    if (x->extype == kint8) {
                        ret = ((int64_t)left < (int64_t)right) ? 'T' : 'F';
                    } else {
                        ret = (left < right) ? 'T' : 'F';
                    }
                } else if (x->type == kle) {
                    if (x->extype == kint8) {
                        ret = ((int64_t)left <= (int64_t)right) ? 'T' : 'F';
                    } else {
                        ret = (left <= right) ? 'T' : 'F';
                    }
                } else if (x->type == kne) {
                    if (x->extype == kint8) {
                        ret = ((int64_t)left != (int64_t)right) ? 'T' : 'F';
                    } else {
                        ret = (left != right) ? 'T' : 'F';
                    }
                } else {
                    ret = 'F';
                }
            }
            put_stack(ret);
        } else {
            return false;
        }
    }

    char c = lg_stack_[0];
    if (c == 'T') return true;
    else if (c == 'F') return false;
    else {
        //printf("error lg_stack_\n");
        return false;
    }
}

bool Record::compare(const char *left, const char *right) const {
    for (size_t i = 0; i < request_.order.size(); i++) {
        order_t ot = request_.order[i];
        const Field& f = meta_->fields_[ot.id];
        if (f.type == kint8) {
            int8_t a = *(int8_t*)(left+f.offset);
            int8_t b = *(int8_t*)(right+f.offset);
            // 如果a=b，不管升降序，都放回false
            return ot.type == 1 ? a < b : a > b;
        } else if (f.type == kint2) {
            int16_t a = *(int16_t*)(left+f.offset);
            int16_t b = *(int16_t*)(right+f.offset);
            return ot.type == 1 ? a < b : a > b;
        } else if (f.type == kint4) {
            int32_t a = *(int32_t*)(left+f.offset);
            int32_t b = *(int32_t*)(right+f.offset);
            return ot.type == 1 ? a < b : a > b;
        } else if (f.type == kint8) {
            int64_t a = *(int64_t*)(left+f.offset);
            int64_t b = *(int64_t*)(right+f.offset);
            return ot.type == 1 ? a < b : a > b;
        } else if (f.type == kfloat) {
            float a = *(float*)(left+f.offset);
            float b = *(float*)(right+f.offset);
            return ot.type == 1 ? a < b : a > b;
        } else if (f.type == kdouble) {
            double a = *(double*)(left+f.offset);
            double b = *(double*)(right+f.offset);
            return ot.type == 1 ? a < b : a > b;
        } else if (f.type == kchar) {
            // a,b:字符串长度
            int16_t a = *(int16_t*)(left+f.offset);
            int16_t b = *(int16_t*)(right+f.offset);
            int16_t min = a < b ? a : b;
            int cmp = memcmp(left+f.offset+2, right+f.offset+2, min);
            if (cmp == 0) {
                return ot.type == 1 ? a < b : a > b;
            } else if (cmp < 0) {
                return ot.type == 1 ? true : false;
            } else {
                return ot.type == 1 ? false : true;
            }
        }
    }
    // 相等返回false
    return false;
}

bool Record::load_and_check_logical(const string& where, string *errmsg) {
    free_logical(lg_);
    if (!load_logical(where.c_str(), where.size(), &lg_, errmsg)) {
        return false;
    }
    if (!check_logical(lg_, errmsg)) {
        return false;
    }
    if (!change_fname_to_fid(lg_, meta_, errmsg)) {
        return false;
    }
    return true;
}

bool Record::opt_get(buffer_t *recv, buffer_t *send) {
    // 请求：[头信息12][字段][keys][where][order][limit]
    // 响应：[数据大小4][状态4][数据条数4][数据体]
    //      [数据体] = [][]
    if (!parse_get_request(recv->data, recv->size)) {
        return_error(send, "GET protocal error");
        return true;
    }
    //printf("[info]where:[%s] keys:%ld data:%ld\n", request_.where.c_str(), request_.keys.size(), data_.size());


    // (1) 找出所有符合条件的记录，key和where双重判断
    // 如果指定key存在，则直接查找，不存在则遍历
    read_data_.clear();
    //printf("keys size:%ld\n", request_.keys.size());
    if (request_.keys.empty()) {
        for (unordered_map<string, char *>::iterator it = data_.begin(); it != data_.end(); it++) {
            read_data_.push_back(it->second);
        }
    } else {
        for (size_t i = 0; i < request_.keys.size(); i++) {
            const string& k = request_.keys[i];
            unordered_map<string, char *>::iterator it = data_.find(k);
            if (it != data_.end()) {
                read_data_.push_back(it->second);
            }
        }
    }

    //printf("[info]where:[%s] count:%ld\n", request_.where.c_str(), read_data_.size());
    // where
    if (request_.where.size() > 0) {
        // 加载条件
        string errmsg;
        if (!load_and_check_logical(request_.where, &errmsg)) {
            return_error(send, errmsg);
            return true;
        }

        // 检查条件
        int size = 0; // 符合条件的数量
        int p = 0;
        while (p < read_data_.size()) {
            char *value = read_data_[p];
            if (compute_logical(value)) {
                // 符合条件的排在前面
                read_data_[size++] = value;
            }
            p++;
        }
        // 后面不符合条件的去掉
        read_data_.resize(size);
    }
    // 排序
    if (request_.order.size() > 0) {
        sort(read_data_.begin(), read_data_.end(), [this](const char *a, const char *b) {
            return compare(a, b);
        });
    }

    printf("[info] keys:%ld where:[%s] result:%ld\n", request_.keys.size(), request_.where.c_str(), read_data_.size());

    // （2）响应：构建返回数据
    // 数据条数
    int offset = request_.offset;
    int size = (int)read_data_.size();
    if (request_.size != -1) size = request_.size;


    // 先申请空间
    int datasize = 0;
    for (int i = 0; i<size && i+offset<read_data_.size(); i++) {
        char *fr = read_data_[i+offset];
        for (size_t j = 0; j < request_.fields.size(); j++) {
            const Field& f = meta_->fields_[request_.fields[j]];
            datasize += f.size;
        }
    }
    if (!expand_buffer(send, datasize)) {
        return_error(send, "memory error");
        return true;
    }

    int data_count = 0;
    char *start = send->data + 12; // 跳过前12个字节
    char *to = start + 4; // 跳过4个字节，长度
    // 提取指定字段，并将字段写入到buffer中
    // 单条记录：[长度][field1][field2]...
    for (int i = 0; i<size && i+offset<read_data_.size(); i++) {
        char *fr = read_data_[i+offset];
        for (size_t j = 0; j < request_.fields.size(); j++) {
            const Field& f = meta_->fields_[request_.fields[j]];
            if (f.type == kchar) {
                int16_t l = *(int16_t*)(fr+f.offset);
                memcpy(to, fr+f.offset, 2+l); to += 2+l;
            } else {
                memcpy(to, fr+f.offset, f.size); to += f.size;
            }
        }
        int sz = to - start - 4; // 单条记录大小
        memcpy(start, &sz, 4);

        // 下条记录开始位置
        start = to;
        to = start + 4;
        data_count++;
    }

    send->size = to - send->data;
    setint4(send->data, send->size - 4);
    setint4(send->data+4, OK);
    setint4(send->data+8, data_count);
    return true;
}

bool Record::parse_del_request(char *buff, int size) {
    char *start = buff + HEADER_SIZE;
    char *p = start;
    if (p - buff > size) return false;
    request_.keys.clear();
    request_.where.clear();

    // keys
    int16_t sz = *(int16_t*)p; p += 2; // keys个数
    for (int i = 0; i < sz; i++) {
        string k;
        p += getchar2(p, &k);
        request_.keys.push_back(k);
    }
    // where
    sz = *(int16_t*)p; p += 2;
    request_.where.assign(p, sz);
    p += sz;
    if (p - buff > size) return false;

    return true;
}

bool Record::opt_del(buffer_t *recv, buffer_t *send) {
    if (!parse_del_request(recv->data, recv->size)) {
        return_error(send, "DEL protocal error");
        return true;
    }
    int count = 0;
    if (request_.keys.size() > 0) {
        // 按key删除数据
        for (size_t i = 0; i < request_.keys.size(); i++) {
            const string& k = request_.keys[i];
            count += data_.erase(k);
        }
    } else if (request_.where.size() > 0) {
        string errmsg;
        if (!load_and_check_logical(request_.where, &errmsg)) {
            return_error(send, errmsg);
            return true;
        }
        // 按条件删除数据
        unordered_map<string, char *>::iterator it = data_.begin();
        while (it != data_.end()) {
            if (compute_logical(it->second)) {
                it = data_.erase(it);
                ++count;
            } else {
                it++;
            }
        }
    }
    setint4(send->data+8, count);
    set_header(send, 4, OK);
    return true;
}

bool Record::parse_agg_request(char *buff, int size) {
    char *start = buff + HEADER_SIZE;
    char *p = start;
    if (p - buff > size) return false;
    // 仅支持：聚合函数、where条件、GroupBy
    request_.aggs.clear();
    request_.where.clear();
    request_.group.clear();

    // 函数
    // [个数][ [函数ID][字段ID], ... ]
    int16_t sz = *(int16_t*)p; p += 2; // 函数个数
    if (sz == 0) return false;
    for (int i = 0; i < sz; i++) {
        agg_t agg;
        agg.aid = *(int16_t*)p; p += 2;
        agg.fid = *(int16_t*)p; p += 2;
        if (agg.fid < 0 || agg.fid > (int16_t)meta_->fields_.size()) {
            return false;
        }
        // 对字符串字段做sum计算，是不合理的，这些判断交由客户端吧
        request_.aggs.push_back(agg);
    }

    // where
    sz = *(int16_t*)p; p += 2;
    request_.where.assign(p, sz);
    p += sz;
    if (p - buff > size) return false;

    // group by
    sz = *(int16_t*)p; p += 2; // 聚合字段个数
    for (int i = 0; i < sz; i++) {
        int16_t id = *(int16_t*)p; p += 2;
        if (id < 0 || id > (int16_t)meta_->fields_.size()) {
            return false;
        }
        request_.group.push_back(id);
    }
    if (p - buff > size) return false;
    return true;
}

typedef struct {
    int16_t aid;   // 函数id
    int16_t fid;   // 字段id
    int16_t ftype; // 字段类型
    int16_t bytes; // 数据大小，double8字节，字符串x字节
    int     count;
    double  sum;
    double  max_d;
    double  min_d;
    string  max_s;
    string  min_s;
} agg_item_t;

typedef vector<agg_item_t> AggList;

void Record::make_key_by_group(const char *value, string *key) {
    if (request_.group.size() == 0) {
        *key = "";
        return;
    }
    for (size_t i = 0; i < request_.group.size(); i++) {
        int16_t id = request_.group[i];
        const Field& f = meta_->fields_[id];
        if (f.type == kchar) {
            int16_t l = *(int16_t*)(value + f.offset);
            key->append(value + f.offset, l+2);
        } else {
            key->append(value + f.offset, f.size);
        }
    }
}

bool Record::opt_agg(buffer_t *recv, buffer_t *send) {
    if (!parse_agg_request(recv->data, recv->size)) {
        return_error(send, "AGG protocal error");
        return true;
    }

    // 符合条件的数据
    read_data_.clear();
    for (unordered_map<string, char *>::iterator it = data_.begin(); it != data_.end(); it++) {
        read_data_.push_back(it->second);
    }

    // where
    if (request_.where.size() > 0) {
        // 加载条件
        string errmsg;
        if (!load_and_check_logical(request_.where, &errmsg)) {
            return_error(send, errmsg);
            return true;
        }

        // 检查条件
        int size = 0; // 符合条件的数量
        int p = 0;
        while (p < read_data_.size()) {
            char *value = read_data_[p];
            if (compute_logical(value)) {
                // 符合条件的排在前面
                read_data_[size++] = value;
            }
            p++;
        }
        // 后面不符合条件的去掉
        read_data_.resize(size);
    }

    // group by
    AggList alist; // 记录聚合函数
    for (size_t i = 0; i < request_.aggs.size(); i++) {
        agg_t x = request_.aggs[i];
        agg_item_t at;
        at.aid = x.aid;
        at.fid = x.fid;
        at.ftype = meta_->fields_[x.fid].type;
        at.count = 0;
        at.bytes = 8;
        at.sum = 0;
        at.max_d = 0;
        at.min_d = 0;
        //printf("func %d %d\n", x.aid, x.fid);
        alist.push_back(at);
    }
    
    // 计算count/sum/avg/max/min
    map<string, AggList> map_data;
    for (size_t i = 0; i < read_data_.size(); i++) {
        const char *value = read_data_[i];
        string key;
        make_key_by_group(value, &key);

        map<string, AggList>::iterator it = map_data.find(key);
        if (it == map_data.end()) {
            AggList t(alist); // 拷贝复制
            map_data[key] = t;
        }

        it = map_data.find(key);
        for (size_t i = 0; i < request_.aggs.size(); i++) {
            agg_item_t& v = it->second[i];
            v.count++; // 不管有没有count，计数+1

            double a;
            string b;
            const Field& f = meta_->fields_[v.fid];
            if (f.type == kint1) {
                a = *(int8_t*)(value + f.offset);
            } else if (f.type == kint2) {
                a = *(int16_t*)(value + f.offset);
            } else if (f.type == kint4) {
                a = *(int32_t*)(value + f.offset);
            } else if (f.type == kint8) {
                a = *(int64_t*)(value + f.offset);
            } else if (f.type == kfloat) {
                a = *(float*)(value + f.offset);
            } else if (f.type == kdouble) {
                a = *(double*)(value + f.offset);
            } else if (f.type == kchar) {
                int16_t l = *(int16_t*)(value + f.offset);
                b.assign(value+f.offset+2, l);
            }

            if (v.aid == 2 || v.aid == 3) {
                // sum/avg
                v.sum += a;
            } else if (v.aid == 4) {
                // max
                if (f.type == kchar) {
                    if (v.max_s < b) v.max_s = b;
                    v.bytes = 2 + v.max_s.size();
                } else {
                    if (v.max_d < a) v.max_d = a;
                }
            } else if (v.aid == 5) {
                // min
                if (f.type == kchar) {
                    if (v.min_s > b) v.min_s = b;
                    v.bytes = 2 + v.min_s.size();
                } else {
                    if (v.min_d > a) v.min_d = a;
                }
            } else {
            }
        }
    }

    // 先申请空间
    int size = 0;
    for (map<string, AggList>::iterator it = map_data.begin(); it != map_data.end(); it++) {
        size += it->first.size();
        for (size_t i = 0; i < it->second.size(); i++) {
            const agg_item_t& v = it->second[i];
            size += v.bytes;
        }
    }
    if (!expand_buffer(send, size)) {
        return_error(send, "memory error");
        return true;
    }

    // 输出
    char *start = send->data + 12; // 跳过前12个字节
    char *to = start + 4; // 跳过4个字节，长度
    // 提取指定字段，并将字段写入到buffer中
    // 单条记录：[长度][field1][field2]...
    for (map<string, AggList>::iterator it = map_data.begin(); it != map_data.end(); it++) {
        // 先输出key
        int size = it->first.size();
        //printf("size:%d\n", size);
        memcpy(to, it->first.data(), size);
        to += size;

        // 再输出value
        for (size_t i = 0; i < it->second.size(); i++) {
            const agg_item_t& v = it->second[i];
            if (v.aid == 1) {
                // count
                memcpy(to, &v.count, 4); to += 4;
            } else if (v.aid == 2) {
                // sum
                memcpy(to, &v.sum, 8); to += 8;
            } else if (v.aid == 3) {
                // avg
                double avg = v.sum/v.count;
                memcpy(to, &avg, 8); to += 8;
            } else if (v.aid == 4) {
                // max
                if (v.ftype == kchar) {
                    to += setchar2(to, v.max_s);
                } else {
                    memcpy(to, &v.max_d, 8); to += 8;
                }
            } else if (v.aid == 5) {
                // min
                if (v.ftype == kchar) {
                    to += setchar2(to, v.min_s);
                } else {
                    memcpy(to, &v.min_d, 8); to += 8;
                }
            }
        }
        int sz = to - start - 4; // 单条记录大小
        memcpy(start, &sz, 4);

        // 下条记录开始位置
        start = to;
        to = start + 4;
    }

    int data_count = map_data.size();
    send->size = to - send->data;
    setint4(send->data, send->size - 4);
    setint4(send->data+4, OK);
    setint4(send->data+8, data_count);
    return true;
}

void Record::load() {
    FILE *rp = fopen(meta_->dump_file_.c_str(), "rb");
    if (rp != NULL) {
        char buf[32];
        while (1) {
            int16_t ksize = 0;
            int sz = fread(&ksize, 2, 1, rp);
            if (sz == 0) break;
            fread(buf, ksize, 1, rp);
            string key(buf, ksize);
            int vsize = 0;
            fread(&vsize, 4, 1, rp);
            // 使用配置文件计算的长度，以适应新增字段
            char *v = create(meta_->record_size_);
            fread(v, vsize, 1, rp);
            data_[key] = v;
        }
        fclose(rp);
    }
}

void Record::put_field_to_buffer() {
    // 申请足够大的空间
    int size = (4 + 32) * (int)meta_->fields_.size();
    field_buffer_ = (char *)malloc(size);

    char *p = field_buffer_;
    for (size_t i = 0; i < meta_->fields_.size(); i++) {
        // [type1][size2][1name]
        const Field& f = meta_->fields_[i];
        int16_t l = f.size;
        if (f.type == kchar) l-=2; // 原因详见meta.cc:87
        p += setint1(p, (int8_t)f.type);
        p += setint2(p, l);
        p += setchar1(p, f.name);
    }
    field_buffer_size_ = p - field_buffer_;
    //printf("field_buffer_size:%d\n", field_buffer_size_);
}
