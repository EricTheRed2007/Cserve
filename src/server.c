#define _WIN32_WINNT 0x0601

#include <string.h>
#include <stdlib.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>

#pragma comment(lib, "ws2_32.lib")


#define BUF_SIZE 4096

const char* get_mime_type(const char *path) {
    if (strstr(path, ".html")) return "text/html";
    if (strstr(path, ".css"))  return "text/css";
    if (strstr(path, ".js"))   return "application/javascript";
    return "text/plain";
}

int is_safe_path(const char *path) {
    return strstr(path, "..") == NULL;
}

void send_response(SOCKET client, const char *status,
                   const char *type, const char *body, int len) {
    char header[512];
    int header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 %s\r\n"
        "Content-Length: %d\r\n"
        "Content-Type: %s\r\n\r\n",
        status, len, type);

    send(client, header, header_len, 0);
    send(client, body, len, 0);
}

void serve_file(SOCKET client, const char *root, const char *url_path) {
    char path[MAX_PATH];

    if (strcmp(url_path, "/") == 0)
        url_path = "/index.html";

    if (!is_safe_path(url_path)) {
        const char *msg = "403 Forbidden";
        send_response(client, "403 Forbidden", "text/plain", msg, strlen(msg));
        return;
    }

    snprintf(path, sizeof(path), "%s%s", root, url_path);

    FILE *file = fopen(path, "rb");
    if (!file) {
        const char *msg = "404 Not Found";
        send_response(client, "404 Not Found", "text/plain", msg, strlen(msg));
        return;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);

    char *data = (char*)malloc(size);
    fread(data, 1, size, file);
    fclose(file);

    send_response(client, "200 OK", get_mime_type(path), data, size);
    free(data);
}

int main(int argc, char *argv[]) {
    int PORT = 8080;
    char *folder = NULL;

    if (argc < 2) {
        printf("Usage: %s <folder> [-p port]\n", argv[0]);
        return 1;
    }

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0) {
            if (i + 1 < argc) {
                PORT = atoi(argv[i + 1]);
                i++;  
            } else {
                printf("Error: -p requires a port number\n");
                return 1;
            }
        }
    }

    if (PORT <= 0 || PORT > 65535) {
        printf("Invalid port number\n");
        return 1;
    }
    
    char root[MAX_PATH];
    GetFullPathNameA(argv[1], MAX_PATH, root, NULL);

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }

    SOCKET server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == INVALID_SOCKET) {
        printf("Socket creation failed\n");
        return 1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    bind(server, (struct sockaddr*)&addr, sizeof(addr));
    listen(server, 10);

    printf("Serving %s on http://localhost:%d\n", root, PORT);

    while (1) {
        SOCKET client = accept(server, NULL, NULL);
        if (client == INVALID_SOCKET)
            continue;

        char request[BUF_SIZE] = {0};
        recv(client, request, sizeof(request) - 1, 0);

        char method[8], path[256];
        sscanf(request, "%s %s", method, path);

        if (strcmp(method, "GET") == 0)
            serve_file(client, root, path);
        else {
            const char *msg = "405 Method Not Allowed";
            send_response(client, "405 Method Not Allowed",
                          "text/plain", msg, strlen(msg));
        }

        closesocket(client);
    }

    closesocket(server);
    WSACleanup();
    return 0;
}
