// g++ -g -o t expression_test3.cc expression.cc
#include <string.h>
#include "expression.h"

int read(char *buff) {
}

int main() {
    char buff[1024];
    FILE *rp = fopen("e.txt", "r");
    if (rp == NULL) return 0;
    fgets(buff, 1024, rp);
    fclose(rp);

    int size = strlen(buff);
    printf("buff:%s\n", buff);

    std::string err;
    expr::Expression ex;
    expr::load_expression(buff, size, &ex, &err);
    printf("load err:%s\n", err.c_str());
    expr::print_expression(ex);

    if (expr::check_expression(ex, &err)) {
        printf("expression is ok\n");
    } else {
        printf("expression is error\n");
        printf("error:%s\n", err.c_str());
    }

    return 0;
}
