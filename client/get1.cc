#include <stdio.h>
#include <string>
#include <vector>

#include "client.h"

using namespace std;

int main(int argc, char *argv[]) {
    Client c;

    Request req;
    /*
    req.fields = "code,name,open,close,high,low,chg,incr,turnover,vol,amt,mv,type";
    req.keys = "000001.SZ,3000001.SZ";
    c.get(req);

    req.fields = "code,name,open,close";
    req.keys = "600519.SH";
    c.get(req);

    */
    req.fields = "*";
    req.keys = "600519.SH,688244.SH";
    c.get(req);

    return 0;
}
