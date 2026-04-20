#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <process.h>
    #include <windows.h>
    #define close_socket closesocket
    #define sleep_ms(ms) Sleep(ms)
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <pthread.h>
    #define close_socket close
    #define sleep_ms(ms) usleep((ms) * 1000)
#endif

#include "server.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <libgen.h>
#include "i18n.h"

static char* load_ui_template() {
    char path[1024] = {0};
#ifdef _WIN32
    char exe_path[MAX_PATH];
    GetModuleFileNameA(NULL, exe_path, MAX_PATH);
    char *last_slash = strrchr(exe_path, '\\');
    if (last_slash) *last_slash = '\0';
    snprintf(path, sizeof(path), "%s\\web\\index.html", exe_path);
#else
    const char *linux_paths[] = {
        "web/index.html",
        "/usr/share/phonedrop/web/index.html",
        "/usr/local/share/phonedrop/web/index.html"
    };
    for (int i = 0; i < 3; i++) {
        struct stat st;
        if (stat(linux_paths[i], &st) == 0) {
            strncpy(path, linux_paths[i], sizeof(path));
            break;
        }
    }
#endif
    FILE *f = fopen(path, "rb");
    if (!f && path[0] == '\0') f = fopen("web/index.html", "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *string = malloc(fsize + 1);
    if (string) {
        size_t read_bytes = fread(string, 1, fsize, f);
        string[read_bytes] = 0;
    }
    fclose(f);
    return string;
}

static void send_file(int client_fd, const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        char *err = "HTTP/1.1 404 Not Found\r\nContent-Length: 14\r\n\r\nFile not found";
        send(client_fd, err, (int)strlen(err), 0);
        return;
    }
    struct stat st;
    stat(path, &st);
    char path_copy[1024];
    strncpy(path_copy, path, 1024);
    char *filename = basename(path_copy);
    char headers[1024];
    int h_len = sprintf(headers,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/octet-stream\r\n"
            "Content-Disposition: attachment; filename=\"%s\"\r\n"
            "Content-Length: %ld\r\n"
            "Connection: close\r\n\r\n",
            filename, (long)st.st_size);
    send(client_fd, headers, h_len, 0);
    char buffer[16384];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (send(client_fd, buffer, (int)bytes, 0) < 0) break;
    }
    fclose(file);
}

#ifdef _WIN32
unsigned __stdcall server_thread_proc(void *arg) {
#else
void* server_thread_proc(void *arg) {
#endif
    PhoneDropServer *server = (PhoneDropServer *)arg;
    while (server->keep_running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = (int)accept(server->socket_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            if (server->keep_running) sleep_ms(10);
            continue;
        }
        char buffer[2048] = {0};
        int received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (received <= 0) { close_socket(client_fd); continue; }
        char method[10] = {0}, path[256] = {0};
        sscanf(buffer, "%s %s", method, path);
        i18n_bundle *bundle = get_bundle(buffer);
        if (strcmp(path, "/") == 0) {
            char *template = load_ui_template();
            char *file_links = malloc(16384);
            memset(file_links, 0, 16384);
            for (int i = 0; i < server->file_count; i++) {
                char link[1024];
                char path_copy[1024];
                strncpy(path_copy, server->file_paths[i], 1024);
                sprintf(link, "<a href='/dl/%d' class='btn btn-download'>%s %s</a>",
                        i, bundle->dl_prefix, basename(path_copy));
                strncat(file_links, link, 16384 - strlen(file_links) - 1);
            }
            char *response = malloc(131072);
            memset(response, 0, 131072);
            int offset = sprintf(response,
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html; charset=UTF-8\r\n"
                "Connection: close\r\n\r\n");
            if (template) {
                char *t = template;
                char *r = response + offset;
                while (*t) {
                    if (strncmp(t, "{{TITLE}}", 9) == 0) { r += sprintf(r, "%s", bundle->title); t += 9; }
                    else if (strncmp(t, "{{FILE_LIST}}", 13) == 0) { r += sprintf(r, "%s", file_links); t += 13; }
                    else if (strncmp(t, "{{EXIT_BTN}}", 12) == 0) { r += sprintf(r, "%s", bundle->exit_btn); t += 12; }
                    else if (strncmp(t, "{{DONATE_TEXT}}", 15) == 0) { r += sprintf(r, "%s", bundle->donate_text); t += 15; }
                    else { *r++ = *t++; }
                }
                *r = '\0';
                free(template);
            } else {
                sprintf(response + offset, "<html><body style='background:#0f0f10;color:#fff;text-align:center;font-family:sans-serif;'>"
                    "<h1>%s</h1><div>%s</div><br><a href='/shutdown'>%s</a></body></html>",
                    bundle->title, file_links, bundle->exit_btn);
            }
            send(client_fd, response, (int)strlen(response), 0);
            free(file_links);
            free(response);
        }
        else if (strncmp(path, "/dl/", 4) == 0) {
            int idx = atoi(path + 4);
            if (idx >= 0 && idx < server->file_count) send_file(client_fd, server->file_paths[idx]);
        }
        else if (strcmp(path, "/shutdown") == 0) {
            char bye[1024];
            sprintf(bye, "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n"
                         "<html><body style='background:#0f0f10;color:#ff453a;text-align:center;padding-top:50px;font-family:sans-serif;'>"
                         "<h1>%s</h1></body></html>", bundle->shutdown_msg);
            send(client_fd, bye, (int)strlen(bye), 0);
            sleep_ms(100);
            server->keep_running = 0;
        }
        close_socket(client_fd);
    }
    return 0;
}

int start_server(PhoneDropServer *server, int start_port) {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return -1;
#endif
    int sockfd = (int)socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) return -1;
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));
    struct sockaddr_in serv_addr;
    int port = start_port;
    int bound = 0;
    while (port < start_port + 100) {
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(port);
        if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == 0) { bound = 1; break; }
        port++;
    }
    if (!bound) { close_socket(sockfd); return -1; }
    listen(sockfd, 5);
    server->port = port;
    server->socket_fd = sockfd;
    server->keep_running = 1;
    return 0;
}

void start_server_thread(PhoneDropServer *server) {
#ifdef _WIN32
    server->thread = (HANDLE)_beginthreadex(NULL, 0, server_thread_proc, server, 0, NULL);
#else
    pthread_create(&server->thread, NULL, server_thread_proc, server);
#endif
}

void stop_server(PhoneDropServer *server) {
    server->keep_running = 0;
    if (server->socket_fd >= 0) {
#ifdef _WIN32
        shutdown(server->socket_fd, SD_BOTH);
        closesocket(server->socket_fd);
#else
        shutdown(server->socket_fd, SHUT_RDWR);
        close(server->socket_fd);
#endif
        server->socket_fd = -1;
    }
#ifdef _WIN32
    if (server->thread) { WaitForSingleObject(server->thread, INFINITE); CloseHandle(server->thread); }
    WSACleanup();
#else
    pthread_join(server->thread, NULL);
#endif
}