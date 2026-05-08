#include <stdio.h>
#include <string>
#include <vector>

#include "client.h"

using namespace std;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("./exe where_condition\n");
        return -1;
    }
    Request req;
    req.fields = "code,name,type,chg,incr,close";
    req.where = argv[1];
    req.order = argv[2];
    req.offset = 0;
    req.size = 50;
    
    Client c;
    c.get(req);

    return 0;
}
