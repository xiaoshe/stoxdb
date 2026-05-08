// g++ -g -o t logical_test1.cc logical.cc expression.cc
#include <string.h>
#include "logical.h"


int main() {
    char buff[1024];
    FILE *rp = fopen("e.txt", "r");
    if (rp == NULL) return 0;
    fgets(buff, 1024, rp);
    fclose(rp);

    int size = strlen(buff);
    printf("buff:%s\n", buff);

    std::string err;
    expr::Logical logic;
    if (!expr::load_logical(buff, size, &logic, &err)) {
        printf("expression is error\n");
        free_logical(logic);
        return 0;
    }

    print_logical(logic);

    if (check_logical(logic, &err)) {
        printf("logical is ok\n");
    } else {
        printf("logical is error\n");
        printf("error:%s\n", err.c_str());
    }

    free_logical(logic);

    return 0;
}
