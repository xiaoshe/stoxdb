#include <stdio.h>

#include "client.h"

int main(int argc, char *argv[]) {
    Client c;

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
    c.end();
    return 0;
}
