#include <stdio.h>

#include "client.h"

int main(int argc, char *argv[]) {
    Client c;

    vector<Field> fields;
    c.Detail(&fields);

    for (size_t i = 0; i < fields.size(); i++) {
        Field& f = fields[i];
        printf("[%ld] %s %s [%s] [%s]\n",
                i, f.name.c_str(), f.rtype.c_str(), f.note.c_str(), f.rform.c_str());
    }

    return 0;
}
