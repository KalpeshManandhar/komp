#include <string.h>
#include <socket.h>
#include <io.h>

int main() {
    int server_fd, client_fd;
    struct sockaddr sockaddress = {{0,0,},{0,0,0,0,0,0,0,0,0,0,0,0,0,0}};  
    const char* msg = "Hello, client!\n";

    // 1. socket(AF_INET, SOCK_STREAM, 0)
    server_fd = socket(2, 1, 0);
    if (server_fd < 0) {
        return 1;
    }

    // 2. bind(server_fd, sockaddress, 16)
    sockaddress.sa_family[0] = 2;        // AF_INET
    sockaddress.sa_data[0] = 0x30;     // Port 12345 (big-endian)
    sockaddress.sa_data[1] = 0x39;     // 12345 in hex

    if (bind(server_fd, &sockaddress, 16) < 0) {
        write(1, "bind failed", strlen("bind failed"));
        return 1;
    }
    
    // 3. listen(server_fd, 5)
    if (listen(server_fd, 5) < 0) {
        write(1, "listen failed", strlen("listen failed"));
        return 1;
    }
    
    // 4. accept(server_fd, NULL, NULL)
    client_fd = accept(server_fd, 0, 0);
    if (client_fd < 0) {
        write(1, "accept failed", strlen("accept failed"));
        return 1;
    }

    // 5. send(client_fd, msg, msglen, 0)
    send(client_fd, msg, strlen(msg), 0);

    // 6. close(client_fd)
    close(client_fd);

    // 7. close(server_fd)
    close(server_fd);

    return 0;
}
