#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "server.h"
#include "gui.h"
#include "net_utils.h"

int main(int argc, char *argv[]) {
#ifndef _WIN32
    signal(SIGPIPE, SIG_IGN);
#endif

    PhoneDropServer my_server = {0};

    my_server.file_paths = malloc(sizeof(char*) * 32);
    if (!my_server.file_paths) return 1;

    my_server.file_count = 0;
    my_server.keep_running = 1;

    if (argc > 1) {
        for (int i = 1; i < argc && i <= 32; i++) {
            my_server.file_paths[my_server.file_count++] = strdup(argv[i]);
        }

        if (start_server(&my_server, 9877) == 0) {
            start_server_thread(&my_server);
            my_server.state = STATE_SERVER_RUNNING;
        }
    } else {
        my_server.state = STATE_HOME;
    }

    char url[128] = {0};
    char ip[16] = {0};
    get_local_ip(ip, sizeof(ip));

    int port = (my_server.port > 0) ? my_server.port : 9877;
    sprintf(url, "http://%s:%d", ip, port);

    run_gui(&my_server, url);

    stop_server(&my_server);

    for (int i = 0; i < my_server.file_count; i++) {
        free(my_server.file_paths[i]);
    }
    free(my_server.file_paths);

    return 0;
}