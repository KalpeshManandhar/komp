#include <string.h>
#include <socket.h>
#include <io.h>

int startsWith(const char *s, const char *prefix) {
    while (*prefix) {
        if (*s != *prefix) {
            return 0;
        }
        s++; prefix++;
    }
    return 1;
}

char* printInt(char* buffer, int a){
    int rev = 0;
    int n = 0;
    while (a > 0){
        rev = rev*10 + a%10;
        a = a/10;
        n++;
    }
    
    while (n > 0){
        *buffer = rev%10 + '0';
        buffer++;
        rev = rev/10;
        n--;
    }

    *buffer = 0;

    return buffer;
}

struct Response{
    int code;
    const char* body;
};

struct Request{
    const char* reqType;
    char route[64];
};


struct Route{
    const char* reqType;
    const char* route;
    struct Response res;
};


char* writeHTMLResponse(char* buffer, struct Response r){
    char *current = buffer; 
    current = strcpy(current, "HTTP/1.1 "); 
    current = printInt(current, r.code);
    
    if (r.code >= 200 && r.code < 300){
        current = strcpy(current, " OK"); 
    }
    else {
        current = strcpy(current, " Not Found"); 
    }
    
    current = strcpy(current, "\r\n"); 

    current = strcpy(current, "Content-Type: text/html\r\n");
    current = strcpy(current, "Content-Length: ");
    current = printInt(current, strlen(r.body));
    
    current = strcpy(current, "\r\n\r\n");
    current = strcpy(current, r.body);
    return current;
}


char* parseRequest(const char* request, struct Request* req){
    const char* current = request;
    if (startsWith(current, "GET ")){
        req->reqType = "GET";
        current = current + strlen("GET ");
    }
    
    const char* untilSpace = current;
    while (*untilSpace != ' '){
        untilSpace++;
    }
    strncpy(&req->route[0], current, untilSpace - current);
    
    return untilSpace;
}



int main() {
    int server_fd, client_fd;
    struct sockaddr sockaddress = {{0,0,},{0,0,0,0,0,0,0,0,0,0,0,0,0,0}};

    // 1. socket(AF_INET, SOCK_STREAM, 0)
    server_fd = socket(2, 1, 0);
    if (server_fd < 0) return 1;

    // 2. bind(server_fd, sockaddress, 16)
    sockaddress.sa_family[0] = 2;      // AF_INET
    sockaddress.sa_data[0] = 0x1f;     // Port 8080 = 0x1f90
    sockaddress.sa_data[1] = 0x90;

    if (bind(server_fd, &sockaddress, 16) < 0) {
        write(1, "bind failed\n", 12);
        return 1;
    }

    // 3. listen(server_fd, 5)
    if (listen(server_fd, 5) < 0) {
        write(1, "listen failed\n", 14);
        return 1;
    }

    // 4. accept(server_fd, NULL, NULL)
    client_fd = accept(server_fd, 0, 0);
    if (client_fd < 0) {
        write(1, "accept failed\n", 14);
        return 1;
    }
    
    
    struct Route routes[4];
    routes[0].reqType = "GET";
    routes[0].route = "/"; 
    routes[0].res.code = 200;
    routes[0].res.body = "<html><body><h1>This is the DEFAULT route.</h1></body></html>";
    
    routes[1].reqType = "GET";
    routes[1].route = "/main"; 
    routes[1].res.code = 200;
    routes[1].res.body = "<html><body><h1>This is the MAIN route.</h1></body></html>";
    
    routes[2].reqType = "GET";
    routes[2].route = "/compiler"; 
    routes[2].res.code = 200;
    routes[2].res.body = "<html><body><h1>This is the COMPILER route.</h1></body></html>";

    routes[3].reqType = "";
    routes[3].route = ""; 
    routes[3].res.code = 404;
    routes[3].res.body = "<html><body><h1>Not found :(</h1></body></html>";
    
    while (1){
        char buffer[1024];
        int n = recv(client_fd, buffer, 1024, 0);
        if (n <= 0) {
            write(1, "recv failed\n", 12);
            return 1;
        }
        
        buffer[n] = 0; // Null-terminate for string handling
        write(1, "Received: \n", strlen("Received: \n"));
        write(1, buffer, strlen(buffer));
        write(1, "\n", strlen("\n"));
        
        struct Request req;
        parseRequest(buffer, &req);
        write(1, "Request: \n", strlen("Request: \n"));
        write(1, req.reqType, strlen(req.reqType));
        write(1, req.route, strlen(req.route));
        write(1, "\n", strlen("\n"));
        
        int i;
        int match = 3;
        for (i=0; i<3; i++){
            if (strcmp(req.reqType, routes[i].reqType) == 0 && strcmp(req.route, routes[i].route) == 0){
                match = i;
                break;
            }
        }
        
        writeHTMLResponse(buffer, routes[match].res);
        write(1, "Response: \n", strlen("Response: \n"));
        write(1, buffer, strlen(buffer));
        write(1, "\n", strlen("\n"));
        
        send(client_fd, buffer, strlen(buffer), 0);
    }
    
    // 6. close(client_fd)
    close(client_fd);
    
    // 7. close(server_fd)
    close(server_fd);

    return 0;
}
