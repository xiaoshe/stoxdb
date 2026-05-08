#include <stdio.h>
#include <string>
#include <vector>

#include "../base.h"
#include "client.h"

using namespace std;

typedef struct {
    string code;
    string name;
    int type;
    int nature;
    float open;
    float close;
    float high;
    float low;
    float chg;
    float incr;
    int64_t vol;
    int64_t amt;
    float turnover;
} stock_t;

float Float(const char *p) {
    float r;
    sscanf(p, "%f", &r);
    return r;
}

int64_t digit64(const char *p) {
    int64_t t = 0;
    while (*p) {
        if (isdigit(*p)) t = t * 10 + *p - '0';
        else break;
        p++;
    }
    return t;
}

void parse(char *p, stock_t *s) {
    string pp(p);
    vector<string> seg;
    strsplit(pp, '\t', &seg);
    s->code = seg[0];
    s->name = seg[1];
    s->type = digit(seg[2].c_str());
    s->nature = digit(seg[3].c_str());
    s->open = Float(seg[4].c_str());
    s->close = Float(seg[5].c_str());
    s->high = Float(seg[6].c_str());
    s->low = Float(seg[7].c_str());
    s->chg = Float(seg[8].c_str());
    s->incr = Float(seg[9].c_str());
    s->vol = digit64(seg[10].c_str());
    s->amt = digit64(seg[11].c_str());
    s->turnover = Float(seg[12].c_str());


    printf("%s %s %d %d %.2f %.2f %.2f %.2f %.2f %.2f %ld %ld %.2f\n", s->code.c_str(),s->name.c_str(),s->type,s->nature,
            s->open, s->close, s->high, s->low, s->chg, s->incr, s->vol, s->amt, s->turnover);
}

int main() {
    Client c("127.0.0.1", 30634, 41401891, false);

    FILE *rp = fopen("stock.txt", "r");
    char buf[4096];
    while (fgets(buf, 4096, rp)) {
        char *p = buf;
        if (*p == '#') continue;
        stock_t s;
        parse(p, &s);

        c.begin();
        c.setk(s.code);
        c.setc("code", s.code);
        c.setc("name", s.name);
        c.seti("type", s.type);
        c.setf("open", s.open);
        c.setf("close", s.close);
        c.setf("high", s.high);
        c.setf("low", s.low);
        c.setf("chg", s.chg);
        c.setf("incr", s.incr);
        c.setf("turnover", s.turnover);
        c.seti("vol", s.vol);
        c.seti("amt", s.amt);
        c.end();
        break;
    }

    fclose(rp);

    /*
    int sz = c.Dump();
    printf("dump ok. sz:%d\n", sz);
    */

    // 更新
    /*
    c.begin();
    c.setk("000558.SZ");
    c.setc("code", "000558.SZ");
    c.setc("name", "天府文旅");
    c.setf("close", 5.21);
    c.setf("chg", -0.2);
    c.seti("type", 101);
    c.seti("status", 3);
    c.seti("mv", 34500000000);
    c.seti("parent_netprofit_ttm", 123456789);
    c.seti("bonus", 987654321);
    c.end();

    c.begin();
    c.setk("600519.SH");
    c.setc("code", "600519.SH");
    c.setc("name", "贵州茅台");
    c.setf("close", 1415.3);
    c.end();


    int sz = c.Dump();
    */


    return 0;
}
