typedef int socklen_t;


struct sockaddr {
    char sa_family[2];
    char sa_data[14];
};


extern int socket(int domain, int type, int protocol);
extern int bind(int sockfd, struct sockaddr * addr, socklen_t addrlen);
extern int listen(int socketfd, int backlog);
extern int accept(int sockfd, struct sockaddr * addr, socklen_t * addrlen);
extern int send(int sockfd, const void* buf, unsigned long size, int flags);
extern int recv(int sockfd, const void* buf, unsigned long size, int flags);
extern int close(unsigned int socket_fd);

