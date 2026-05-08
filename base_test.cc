#include "base.h"

int main() {
    string v = "ST汉缆股份";
    string v1 = "";

    if (contain(v, v1)) {
        printf("True\n");
    } else {
        printf("False\n");
    }
    if (startwith(v, v1)) {
        printf("True\n");
    } else {
        printf("False\n");
    }
    v1 = "股份";
    if (endwith(v, v1)) {
        printf("True\n");
    } else {
        printf("False\n");
    }
    return 0;
}
