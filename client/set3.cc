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
    Client c;

    // 写入100个数据
    int count  = 0;
    FILE *rp = fopen("stock.txt", "r");
    char buf[4096];
    while (fgets(buf, 4096, rp)) {
        char *p = buf;
        if (*p == '#') continue;
        stock_t s;
        parse(p, &s);

        c.begin();
        c.setkey(s.code);
        c.setvalue("code", s.code);
        c.setvalue("name", s.name);
        c.setvalue("type", s.type);
        c.setvalue("open", s.open);
        c.setvalue("close", s.close);
        c.setvalue("high", s.high);
        c.setvalue("low", s.low);
        c.setvalue("chg", s.chg);
        c.setvalue("incr", s.incr);
        c.setvalue("turnover", s.turnover);
        c.setvalue("vol", s.vol);
        c.setvalue("amt", s.amt);
        c.end();
        if (count++ == 100)break;
    }

    fclose(rp);

    int sz = c.Dump();
    printf("dump %d\n", sz);

    return 0;
}
