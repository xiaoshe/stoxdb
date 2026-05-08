#include "meta.h"
#include "record.h"
#include "server.h"
#include <stdio.h>
#include <string.h>
#include <signal.h>

static int running = 1;

void handle_signal(int sig) {
    running = 0;
}

int main() {
    Meta meta;
    if (!meta.load_config()) {
        return 1;
    }
    meta.info();
    //meta.desc();

    Record r(&meta);
    //r.data();

    // 设置信号处理
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    Server server(&meta, &r);
    if (!server.init()) {
        return 2;
    }
    server.start(&running);
    
    return 0;
}
