#include "expression.h"
#include <cmath>

namespace expr {

// 比较符号
bool is_compare_type(Type type) {
    return keq <= type && type <= kne;
}

// like notlike
bool is_like_type(Type type) {
    return type == klike || type == knlike;
}

// 四则运算
bool is_four_type(Type type) {
    return kadd <= type && type <= kmod;
}

bool is_logical_type(Type type) {
    return type == kand || type == kor;
}

bool is_digit_type(Type type) {
    return kint1 <= type && type <= kdouble;
}

// 从字符串开头提取浮点数或整数，返回使用到的数字长度
// 如果数字有小数点，则为浮点数。否则为整数
int get_double(const char *str, int size, double *d, Type *type) {
    double a = 0, b = 0; // a整数，b小数
    int i = 0;
    while (i < size) {
        char c = str[i];
        if ('0' <= c && c <= '9') {
            a = a * 10 + c - '0';
            i++;
        } else {
            break;
        }
    }
    *type = kint8;
    if (i < size && str[i] == '.') {
        *type = kdouble;
        i++;
        while (i < size) {
            char c = str[i];
            if ('0' <= c && c <= '9') {
                b = b * 10 + c - '0';
                i++;
            } else {
                break;
            }
        }
    }
    while (b >= 1) b /= 10;
    *d = a + b;
    return i;
}

// 从字符串开头提取字段名（字母、数字、下划线），返回使用到的字符长度
int get_name(const char *str, int size, string *name) {
    int i = 0;
    while (i < size) {
        char p = str[i];
        if ('A' <= p && p <= 'Z') p += 32;
        if (('a' <= p && p <= 'z') || ('0' <= p && p <= '9') || p == '_') {
            name->push_back(p);
            i++;
        } else {
            break;
        }
    }
    return i;
}


bool load_expression(const char *str, int size, Expression *ex, string *errmsg) {
    int i = 0;
    while (i < size) {
        char c = str[i];
        if (c == ' ' || c == '\t' || c == '\n') {
            // 跳过空格等
            i++;
            continue;
        }

        Node *p = new Node();
        //printf("newNode\n");
        if (p == NULL) {
            errmsg->assign("not enough memory");
            return false;
        }
        if ('A' <= c && c <= 'Z') c += 32;
        if (c == '(') {
            p->type = kleft;
            ex->push_back(p);
            i++;
        } else if (c == ')') {
            p->type = kright;
            ex->push_back(p);
            i++;
        } else if (c == '+') {
            p->type = kadd;
            ex->push_back(p);
            i++;
        } else if (c == '-') {
            // 判断是不是负数
            int sz = ex->size();
            if (sz>0 && (ex->at(sz-1)->type == kname || ex->at(sz-1)->type == kright)) {
                // 如果前面是字段，或者右括号，则认为是减号
                p->type = ksub;
                ex->push_back(p);
                i++;
            } else {
                i++;
                double d;
                i += get_double(str+i, size-i, &d, &(p->type));
                p->vd = 0-d;
                ex->push_back(p);
            }
        } else if (c == '*') {
            p->type = kmulti;
            ex->push_back(p);
            i++;
        } else if (c == '/') {
            p->type = kdiv;
            ex->push_back(p);
            i++;
        } else if (c == '%') {
            p->type = kmod;
            ex->push_back(p);
            i++;
        } else if (c == '>') {
            if (i+1 < size && str[i+1] == '=') {
                p->type = kge;
                ex->push_back(p);
                i += 2;
            } else {
                p->type = kgt;
                ex->push_back(p);
                i++;
            }
        } else if (c == '<') {
            if (i+1 < size && str[i+1] == '=') {
                p->type = kle;
                ex->push_back(p);
                i += 2;
            } else {
                p->type = klt;
                ex->push_back(p);
                i++;
            }
        } else if (c == '=') {
            p->type = keq;
            ex->push_back(p);
            i++;
        } else if (c == '!') {
            if (i+1 < size && str[i+1] == '=') {
                p->type = kne;
                ex->push_back(p);
                i += 2;
            } else {
                p->type = kerror;
                ex->push_back(p);
                errmsg->assign(str+i, 1);
                return false;
            }
        } else if ('a' <= c && c <= 'z') {
            // 字段名，或关键字and/or/like
            string name;
            i += get_name(str+i, size-i, &name);
            if (name == "like") {
                p->type = klike;
                ex->push_back(p);
            } else if (name == "notlike") {
                p->type = knlike;
                ex->push_back(p);
            } else if (name == "and") {
                p->type = kand;
                ex->push_back(p);
            } else if (name == "or") {
                p->type = kor;
                ex->push_back(p);
            } else {
                p->type = kname;
                p->vs = name;
                ex->push_back(p);
            }
        } else if ('0' <= c && c <= '9') {
            i += get_double(str+i, size-i, &(p->vd), &(p->type));
            ex->push_back(p);
        } else if (c == '\'') {
            // 字符串数值
            i++;
            int s = i;
            int e = i;
            while (i < size) {
                if (str[i] == '\'') {
                    e = i++;
                    break;
                } else {
                    i++;
                }
            }
            p->type = kchar;
            p->vs.assign(str+s, e-s);
            ex->push_back(p);
        } else {
            p->type = kerror;
            ex->push_back(p);
            errmsg->assign(str+i, 1);
            return false;
        }
    }

    return true;
}


void print_expression(const Expression& ex) {
    printf("Expression: ");
    for (int i = 0; i < ex.size(); i++) {
        Node *p = ex[i];
        if (p->type == kname || p->type == kchar) {
            printf("%s ", p->vs.c_str());
        } else if (p->type == kdouble || p->type == kfloat) {
            printf("%f(%s) ", p->vd, gtype[p->type]);
        } else if (p->type == kid) {
            printf("[%d] ", p->id);
        } else if (p->type >= kint1 && p->type <= kint8) {
            printf("%ld(%s) ", (int64_t)p->vd, gtype[p->type]);
        } else {
            printf("%s ", gtype[p->type]);
        }
    }
    printf("\n");
}

bool check_expression(const Expression& ex, string *errmsg) {
    // (1)小括号必须配对
    // (2)+- * /%两边必须有数字
    int left=1, opt=2, digit=3; // 1左括号 2操作 3数字
    vector<int> stack;
    stack.reserve(8);
    for (int i = 0; i < ex.size(); i++) {
        Node *x = ex[i];
        if (x->type == kleft) {
            // 左括号，直接入栈
            stack.push_back(left);
        } else if (x->type == kright) {
            // 右括号，前面2个出栈后入栈数字，相当于:(数字) -> 数字
            // 栈内必须以 (+数字 结尾
            int sz = stack.size();
            if (sz < 2) {
                errmsg->assign("maybe lack '('");
                return false;
            }
            int a = stack[sz-1];
            int b = stack[sz-2];
            stack.resize(sz-2);
            if (a != digit || b != left) {
                errmsg->assign("error before ')'");
                return false;
            }

            stack.push_back(digit);
        } else if (is_four_type(x->type)) {
            // 运算符，在有数字的情况下，入栈
            if (stack.empty() || stack.back() != digit) {
                errmsg->assign(string("error besize operator:") + gtype[x->type]);
                return false;
            }
            stack.push_back(opt);
        } else if (x->type == kname || is_digit_type(x->type)) {
            // 出栈2个元素，相当于：数字1+运算符+数字2 -> 数字3
            if (!stack.empty() && stack.back() == opt) {
                stack.pop_back();
                stack.pop_back();
            }
            stack.push_back(digit);
        } else {
            errmsg->assign(string("unsupported type:")+gtype[x->type]);
            return false;
        }
    }

    // 栈内只剩一个数字
    return stack.size() == 1 && stack[0] == digit;
}

void free_expression(const Expression& ex) {
    for (int i = 0; i < ex.size(); i++) {
        //printf("deleteNode\n");
        delete ex[i];
    }
}


Computer::Computer() {
}

Computer::~Computer() {
}

void Computer::init() {
    data.clear();
}

void Computer::push_opt(Type t) {
    if (t == kright) {
        // 弹出前一个左括号及其中间的数字，并计算
        // (a+b-c) -> d
        vector<CNode> st;
        st.reserve(8);
        while (data.size() > 0) {
            CNode n = data[data.size()-1]; // 复制
            data.resize(data.size()-1);
            if (n.type == kleft) break;
            st.push_back(n);
        }
        double v = 0;
        bool add = true;
        while (st.size() > 0) {
            CNode n = st[st.size()-1];
            st.resize(st.size()-1);
            if (n.type == kadd) {
                add = true;
            } else if (n.type == ksub) {
                add = false;
            } else {
                if (add) v += n.v;
                else v -= n.v;
            }
        }
        push_number(v);
    } else {
        data.push_back(CNode(t));
    }
}

void Computer::push_number(double v) {
    // 如果上一个unit是乘法或除法或求余，弹出数字并计算，然后入栈
    int sz = data.size();
    if (sz > 1) {
        const CNode& n = data[sz-1];
        if (n.type == kmulti || n.type == kdiv || n.type == kmod) {
            CNode a = data[sz-2];
            data.resize(sz-2);
            if (n.type == kmulti) {
                data.push_back(CNode(kdouble, a.v*v));
            } else if (n.type == kdiv) {
                double t = a.v/v;
                if (isinf(t)) t = 0;
                data.push_back(CNode(kdouble, t));
            } else {
                int64_t t = (int64_t)a.v % (int64_t)v;
                data.push_back(CNode(kdouble, (double)t));
            }
        } else {
            data.push_back(CNode(kdouble, v));
        }
    } else {
        data.push_back(CNode(kdouble, v));
    }
}

double Computer::result() {
    double v = 0;
    bool add = true;
    for (int i = 0; i < data.size(); i++) {
        const CNode& u = data[i];
        if (u.type == kadd) {
            add = true;
        } else if (u.type == ksub) {
            add = false;
        } else {
            if (add) v += u.v;
            else v -= u.v;
        }
    }
    return v;
}
}
