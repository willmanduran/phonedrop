#ifndef SERVER_H
#define SERVER_H

#ifdef _WIN32
    #include <windows.h>
    typedef HANDLE thread_t;
#else
#include <pthread.h>
typedef pthread_t thread_t;
#endif

typedef enum {
    STATE_HOME,
    STATE_SERVER_RUNNING
} AppState;

typedef struct {
    int port;
    int socket_fd;
    char **file_paths;
    int file_count;
    int keep_running;
    thread_t thread;
    AppState state;
} PhoneDropServer;

int start_server(PhoneDropServer *server, int start_port);
void start_server_thread(PhoneDropServer *server);
void stop_server(PhoneDropServer *server);

#endif