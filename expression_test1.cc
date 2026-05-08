// g++ -g -o t expression_test1.cc expression.cc
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
    if (!expr::load_expression(buff, size, &ex, &err)) {
        printf("load err:%s\n", err.c_str());
        return 0;
    }

    expr::print_expression(ex);
    //expr::print_list(l2);

    /*
    double d;
    int i;

    buff = "123.4567";
    i = expr::get_double(buff, 8, &d);
    printf("i=%d d=%.6f\n", i, d);

    buff = "12345678";
    i = expr::get_double(buff, 8, &d);
    printf("i=%d d=%.6f\n", i, d);

    buff = "1234.5678 )";
    i = expr::get_double(buff, 10, &d);
    printf("i=%d d=%.6f\n", i, d);
    */

    return 0;
}
