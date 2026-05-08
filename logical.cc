#include "logical.h"

namespace expr {


LogicalNode *convert(Node *in) {
    LogicalNode *n = new LogicalNode(); // 不能使用malloc
    //printf("newLogicalNode\n");
    n->type = in->type;
    //printf("deleteNode\n");
    delete in;
    return n;
}

// 正向，exp的前面是表达式，只要找到截止位置即可
int make_right_expression(const Expression& in, int start, Expression& out) {
    int left = 0;
    int p = start;
    while (p < in.size()) {
        Node *x = in[p];
        if (x->type == kand ||                  // a > (b+c) and 
            x->type == kor ||                   // a > (b+c) or
            (x->type == kright && left == 0)    // (a=0 or a=(b+c))
            ) {
            // 截止位置
            break;
        } else {
            if (x->type == kleft) {
                left++;
            } else if (x->type == kright) {
                left--;
            }
            out.push_back(x);
            p++;
        }
    }
    return p;
}

// 反向，st的后面是表达式，向前找到截止位置
void make_left_expression(vector<Node*>& st, Expression& ex) {
    int right = 0; // 右括号的数量
    while (st.size() > 0) {
        Node *x = st.back();
        if (x->type == kand ||                  // a > (b+c) and 
            x->type == kor ||                   // a > (b+c) or
            (x->type == kleft && right == 0)    // (a=0 or a=(b+c))
            ) {
            // 截止位置
            break;
        } else {
            if (x->type == kright) {
                right++;
            } else if (x->type == kleft) {
                right--;
            }
            st.pop_back();
            ex.push_back(x);
        }
    }
    // 前后对调
    int i = 0;
    int j = ex.size()-1;
    while (i < j) {
        Node *x = ex[i];
        ex[i] = ex[j];
        ex[j] = x;
        i++, j--;
    }
}


// (a+1>b or c=0) and d<0
bool load_logical(const char *str, int size, Logical *lg, string *errmsg) {
    Expression ex;
    ex.reserve(16);
    if (!load_expression(str, size, &ex, errmsg)) {
        free_expression(ex);
        return false;
    }

    // 拆分expression
    vector<Node*> st; // 类型同Expression
    st.reserve(16);
    int i = 0;
    while (i < ex.size()) {
        Node *x = ex[i];
        if (is_compare_type(x->type) || is_like_type(x->type)) {
            // 比较符号 || like
            LogicalNode *n = convert(x);

            // 左边是表达式，在st的后面
            make_left_expression(st, n->left);

            // st中剩余的需要放到data中
            for (int j = 0; j < st.size(); j++) {
                lg->push_back(convert(st[j]));
            }
            st.clear();

            // 右边是表达式，在ex的前面
            i = make_right_expression(ex, i+1, n->right);

            lg->push_back(n);
        } else {
            st.push_back(x);
            i++;
        }
    }

    // st中剩余的需要放到data中
    for (int j = 0; j < st.size(); j++) {
        lg->push_back(convert(st[j]));
    }
    st.clear();

    // ex中的Node*已经转移，不需释放ex

    for (int i = 0; i < lg->size(); i++) {
        LogicalNode *n = (*lg)[i];
        if (is_like_type(n->type)) {
            // LIKE区分：开头^和结尾$
            n->like_start = false;
            n->like_end = false;
            string& v1 = n->right[0]->vs;
            int sz = v1.size();
            if (sz > 0) {
                if (v1[sz-1] == '$') {
                    v1.resize(sz-1);
                    n->like_end = true;
                    sz--;
                }
                if (sz > 0 && v1[0] == '^') {
                    v1.erase(0, 1);
                    n->like_start = true;
                    sz--;
                }
            }
        }
    }

    return true;
}

void print_logical(const Logical& lg) {
    for (int i = 0; i < lg.size(); i++) {
        LogicalNode *x = lg[i];
        printf("\n[%d]LogicalNode:%s\n", i, gtype[x->type]);
        if (is_compare_type(x->type) || is_like_type(x->type)) {
            print_expression(x->left);
            print_expression(x->right);
            if (is_like_type(x->type)) {
                printf("[LIKE]start:%d end:%d\n", x->like_start, x->like_end);
            }
        }
    }
}

void push_expr(vector<int>& st) {
    // 入栈表达式时，如果前面是：expr AND，则出栈前面2个后入栈
    int sz = st.size();
    if (sz > 1) {
        if (st[sz-1] == 2 && st[sz-2] == 3) {
            st.resize(sz - 2);
        }
    }
    st.push_back(3);
}

bool check_logical(const Logical& lg, std::string *errmsg) {
    // (1)小括号必须配对
    // (2)and or两边必须有表达式
    int left=1, opt=2, expr=3; // 1左括号 2操作 3表达式
    vector<int> st;
    st.reserve(16);
    for (int i = 0; i < lg.size(); i++) {
        LogicalNode *x = lg[i];
        //printf("LogicalNode type:%d\n", x->type);
        if (x->type == kleft) {
            // 左括号，直接入栈
            st.push_back(left);
        } else if (x->type == kright) {
            // 右括号，前面2个出栈后入栈表达式，相当于:(表达式) -> 表达式
            // 栈内必须以 (+表达式 结尾
            if (st.size() < 2) {
                errmsg->assign("maybe lack '('");
                return false;
            }
            int a = st.back();
            st.pop_back();
            int b = st.back();
            st.pop_back();
            if (a != expr || b != left) {
                errmsg->assign("error before ')'");
                return false;
            }

            push_expr(st);
        } else if (is_logical_type(x->type)) {
            // 逻辑运算符，在有表达式的情况下，入栈
            if (st.empty() || st.back() != expr) {
                errmsg->assign(std::string("error besize operator:") + gtype[x->type]);
                return false;
            }
            st.push_back(opt);
        } else if (is_compare_type(x->type)) {
            // 比较表达式，有2种，一种数字表达式，一种字符串
            if (x->left.size() == 1 && x->left[0]->type == kname && x->right.size() == 1 && x->right[0]->type == kchar) {
                // 字符串，左右表达式只能有一个字段
            } else {
                // 表达式，出栈2个元素，相当于：表达式1 + and(or) + 表达式2 -> 表达式3
                if (!check_expression(x->left, errmsg)) {
                    errmsg->assign("left expression invalid");
                    return false;
                }
                if (!check_expression(x->right, errmsg)) {
                    errmsg->assign("right expression invalid");
                    return false;
                }
            }
            if (!st.empty() && st.back() == opt) {
                st.pop_back();
                st.pop_back();
            }
            push_expr(st);
        } else if (is_like_type(x->type)) {
            // 左边必须只有一个kname
            if (x->left.size() != 1 || x->left[0]->type != kname) {
                errmsg->assign("left of LIKE is not field");
                return false;
            }
            if (x->right.size() != 1 || x->right[0]->type != kchar) {
                errmsg->assign("right of LIKE is not string");
                return false;
            }
            if (!st.empty() && st.back() == opt) {
                st.pop_back();
                st.pop_back();
            }
            push_expr(st);
        } else {
            errmsg->assign(std::string("not logical type:")+gtype[x->type]);
            return false;
        }
    }

    // 栈内只剩一个数字
    if (st.size() == 1 && st[0] == expr) {
        return true;
    } else {
        errmsg->assign("loss expression");
        return false;
    }
}

void free_logical(Logical& lg) {
    for (int i = 0; i < lg.size(); i++) {
        // 释放空间
        LogicalNode *x = lg[i];
        free_expression(x->left);
        free_expression(x->right);
        //printf("deleteLogicalNode\n");
        delete x;
    }
    lg.clear();
}

}
