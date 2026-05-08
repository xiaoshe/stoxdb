#include <stdio.h>

#include "client.h"

int main(int argc, char *argv[]) {
    Client c;

    vector<Field> fields;
    c.Column(&fields);

    for (size_t i = 0; i < fields.size(); i++) {
        Field& f = fields[i];
        printf("[%ld] type:%d:%s name:%s size:%d\n", i, f.type, gtype[f.type], f.name.c_str(), f.size);
    }

    return 0;
}
