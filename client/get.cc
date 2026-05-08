#include <stdio.h>
#include <string>
#include <vector>

#include "client.h"

using namespace std;

int main(int argc, char *argv[]) {
    Client c("127.0.0.1", 30634, 41401891);

    int size = 10;
    c.get("code,name,open,close,high,low,chg,incr,turnover,vol,amt,mv,type", "", "", "", 0, size);
    printf("===========\n");
    c.get("code, NAME,open ,close,high,low,chg,incr,turnover,vol,amt,mv,type", "300850.SZ, 600135.SH, 001268.SZ");

    return 0;
}
