#include <stdio.h>
#include <string>
#include <vector>

#include "client.h"

using namespace std;

int main(int argc, char *argv[]) {
    Client c;

    c.get("code,name1", "", "", "", 0, 10);

    c.get("code,name,open,close,high,low,chg,incr,turnover,vol,amt,mv,type", "", "", "", 0, 10);


    return 0;
}
