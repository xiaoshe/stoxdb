// g++ -g -o a expression_test2.cc expression.cc
#include "expression.h"
#include <stdio.h>
#include <string.h>

using namespace expr;

int main() {
    {
        Computer c;
        double a = 1-3+4;
        printf("1-3+4 = %.2f\n", a);
        c.push_number(1); // 1
        c.push_opt(ksub); // -
        c.push_number(3); // 3
        c.push_opt(kadd); // +
        c.push_number(4); // 4
        printf("result=%.2f\n", c.result());
    }
    {
        Computer c;
        double a = (1+5)/3;
        printf("\n(1+5)/3 = %.2f\n", a);
        c.push_opt(kleft);  // (
        c.push_number(1);   // 1
        c.push_opt(kadd);   // +
        c.push_number(5);   // 5
        c.push_opt(kright); // )
        c.push_opt(kdiv);   // /
        c.push_number(3);   // 3
        printf("result=%.2f\n", c.result());

        a = 1-3;
        printf("\n1-3 = %.2f\n", a);
        c.init();
        c.push_number(1);   // 1
        c.push_opt(ksub);   // -
        c.push_number(3);   // 3
        printf("result=%.2f\n", c.result());
    }

    {
        Computer c;
        double a = (5+(4-3)*2+1)+(100/3.0);
        printf("\n(5+(4-3)*2+1)+(100/3) = %.2f\n", a);
        c.push_opt(kleft);  // (
        c.push_number(5);   // 5
        c.push_opt(kadd);   // +
        c.push_opt(kleft);  // (
        c.push_number(4);   // 4
        c.push_opt(ksub);   // -
        c.push_number(3);   // 3
        c.push_opt(kright); // )
        c.push_opt(kmulti); // *
        c.push_number(2);   // 2
        c.push_opt(kadd);   // +
        c.push_number(1);   // 1
        c.push_opt(kright); // )
        c.push_opt(kadd);   // +
        c.push_opt(kleft);  // (
        c.push_number(100); // 100
        c.push_opt(kdiv);   // /
        c.push_number(3);   // 3
        c.push_opt(kright); // )
        printf("result=%.2f\n", c.result());
    }

    {
        Computer c;
        double a = (5-4)/4.0;
        printf("\n(5-4)/4 = %.2f\n", a);
        c.push_opt(kleft);  // (
        c.push_number(5);   // 5
        c.push_opt(ksub);   // -
        c.push_number(4);   // 4
        c.push_opt(kright); // )
        c.push_opt(kdiv);   // /
        c.push_number(4);   // 4
        printf("result=%.2f\n", c.result());
    }

    {
        Computer c;
        int a = (2+8) % 3;
        printf("\n(2+8) mod 3 = %d\n", a);
        c.push_opt(kleft);  // (
        c.push_number(2);   // 2
        c.push_opt(kadd);   // +
        c.push_number(8);   // 8
        c.push_opt(kright); // )
        c.push_opt(kmod);   // %
        c.push_number(3);   // 3
        printf("result = %d\n", (int)c.result());
    }

    {
        Computer c;
        // 1/0
        c.push_number(1);
        c.push_opt(kdiv);
        c.push_number(0);
        printf("1/0 = %f\n", c.result());
    }

    return 0;
}
