#include "base.h"

#include <string.h>

int scanconf(const vector<string>& conf, const string& key) {
    for (size_t i = 0; i < conf.size(); i++) {
        if (conf[i] == key) return (int)i;
    }
    return -1;
}

void strsplit(const string& str, char ch, vector<string> *out) {
    const char *p = str.c_str();
    string s;
    while (*p) {
        if (*p == ch) {
            out->push_back(s);
            s.clear();
        } else {
            s.push_back(*p);
        }
        p++;
    }
    if (!s.empty()) out->push_back(s);
}

void trim(string& str) {
    // 移除开头空格
    str.erase(0, str.find_first_not_of(" \t\n\r\f\v"));
    // 移除末尾空格
    str.erase(str.find_last_not_of(" \t\n\r\f\v") + 1);
}

void tolower(string& str) {
    for (size_t i = 0; i < str.size(); i++) {
        char c = str[i];
        if (c >= 'A' && c <= 'Z') {
            str[i] = c + 32;
        }
    }
}

void print(const vector<string> &v) {
    for (size_t i = 0; i < v.size(); i++) {
        printf("  %s\n", v[i].c_str());
    }
}

int digit(const char *p) {
    int t = 0;
    while (*p) {
        if (isdigit(*p)) t = t * 10 + *p - '0';
        else break;
        p++;
    }
    return t;
}

bool isopt(char p) {
    return  p == '(' ||
            p == ')' ||
            p == '+' ||
            p == '-' ||
            p == '*' ||
            p == '/';
}
bool isdigit(char p) {
    return p >= '0' && p <= '9';
}
bool isalpha(char p) {
    return (p >= 'a' && p <= 'z') || (p >= 'A' && p <= 'Z');
}


int setint1(char *buff, int8_t v) {
    memcpy(buff, &v, 1);
    return 1;
}

int setint2(char *buff, int16_t v) {
    memcpy(buff, &v, 2);
    return 2;
}

int setint4(char *buff, int v) {
    memcpy(buff, &v, 4);
    return 4;
}

int setint8(char *buff, int64_t v) {
    memcpy(buff, &v, 8);
    return 8;
}

int setfloat(char *buff, float v) {
    memcpy(buff, &v, 4);
    return 4;
}

int setdouble(char *buff, double v) {
    memcpy(buff, &v, 8);
    return 8;
}

int setchar1(char *buff, const string& input) {
    uint8_t l = (uint8_t)input.length();
    memcpy(buff, &l, 1);
    memcpy(buff+1, input.c_str(), l);
    return l+1;
}

int setchar2(char *buff, const string& input) {
    uint16_t l = (uint16_t)input.length();
    memcpy(buff, &l, 2);
    memcpy(buff+2, input.c_str(), l);
    return l+2;
}

int getint1(const char *p, int8_t *out) {
    *out = *(int8_t*)p;
    return 1;
}

int getint2(const char *p, int16_t *out) {
    *out = *(int16_t*)p;
    return 2;
}

int getint4(const char *p, int32_t *out) {
    *out = *(int32_t*)p;
    return 4;
}

int getint8(const char *p, int64_t *out) {
    *out = *(int64_t*)p;
    return 8;
}

int getchar1(const char *p, string *out) {
    uint8_t l = *(uint8_t*)p;
    out->assign(p+1, l);
    return l+1;
}

int getchar2(const char *p, string *out) {
    uint16_t l = *(uint16_t*)p;
    out->assign(p+2, l);
    return l+2;
}

bool contain(const string& str, const string& sub) {
    size_t l1 = str.size();
    size_t l2 = sub.size();
    if (l1 < l2) return false;
    return str.find(sub) != string::npos;
}

bool startwith(const string& str, const string& sub) {
    size_t l1 = str.size();
    size_t l2 = sub.size();
    if (l1 < l2) return false;
    for (size_t i = 0; i < l2; i++) {
        if (str[i] != sub[i]) return false;
    }
    return true;
}

bool endwith(const string& str, const string& sub) {
    size_t l1 = str.size();
    size_t l2 = sub.size();
    if (l1 < l2) return false;
    for (size_t i = 0; i < l2; i++) {
        if (str[i+l1-l2] != sub[i]) return false;
    }
    return true;
}
