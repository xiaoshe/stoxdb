#include <stdio.h>

#include "client.h"

int main(int argc, char *argv[]) {
    Client c;

    printf("dump %d\n", c.Dump());

    return 0;
}
