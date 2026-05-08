#include <stdio.h>

#include "client.h"

int main(int argc, char *argv[]) {
    Client c;

    // 在begin和end之间写入2条数据
    c.begin();

    c.setkey("000001.SZ");
    c.setvalue("code", "000001.SZ");
    c.setvalue("name", "平安银行");
    c.setvalue("close", 11.12);
    c.setvalue("chg", -0.07);
    c.setvalue("status", 1);
    c.setvalue("type", 102);
    c.setvalue("date", 20260119);
    c.setvalue("mv", 215800000000);

    c.setkey("600519.SH");
    c.setvalue("code", "600519.SH");
    c.setvalue("name", "贵州茅台");
    c.setvalue("close", (double)1376);
    c.setvalue("chg", (double)-6);

    c.end();
    return 0;
}
