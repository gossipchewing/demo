#define _GNU_SOURCE 1
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <poll.h>

#define FD_LIMIT 3 // 文件描述符数量限制
#define USER_LIMIT 3 // 最大用户数量限制
#define BUFFER_SIZE 64 // 读缓冲区大小

struct client_data
{
    // 用gcc编译的时候要写成：struct sockaddr_in address； g++ 则不用
    sockaddr_in address; // 客户端 socket 地址
    char* write_buf; // 待写到客户端的数据的位置
    char buf[BUFFER_SIZE]; // 从客户端读入的数据
};

int setnoblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);

    return old_option;
}

int main(int argc, char* argv[])
{
    if(argc < 2) {
        printf("usage: %s ip_address port_number \n", basename(argv[0]));
        return 1;
    }

    const char* ip = argv[1];
    int port = atoi(argv[2]);

    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    // int inet_pton(int family, const char *strptr, void *addrptr); 函数中p和n分别代表表达（presentation)和数值（numeric)。地址的表达格式通常是ASCII字符串（如点分十进制形式），数值格式则是存放到套接字地址结构的二进制值。
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port); // htons 是将整型变量从主机字节顺序转变成网络字节顺序

    // int socket(int domain, int type, int protocol);
    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);

    // int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
    ret = bind(listenfd, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    // int listen(int sockfd, int backlog);
    ret = listen(listenfd, 2); // backlog 貌似没啥用~~
    assert(ret != -1);

    client_data* users = new client_data[FD_LIMIT];
    pollfd fds[USER_LIMIT + 1];
    int user_counter = 0;
    for(int i = 1; i <= USER_LIMIT; ++i) {
        fds[i].fd = -1;
        fds[i].events = 0;
    }

    fds[0].fd = listenfd;
    fds[0].events = POLLIN | POLLERR;
    fds[0].revents = 0; // revents 是由 poll 设置的

    while(1) {
        // int poll ( struct pollfd * fds, unsigned int nfds, int timeout);
        ret = poll(fds, user_counter + 1, -1);
        if(ret < 0) {
            printf("poll failure\n");
            break;
        }

        for(int i = 0; i < user_counter+1; ++i) {
            if( (fds[i].fd == listenfd) && (fds[i].revents & POLLIN) ) {
                struct sockaddr_in client_address;
                socklen_t client_addr_length = sizeof(client_address);
                int connfd = accept(listenfd, (struct sockaddr*)&client_address, &client_addr_length);

                if(connfd < 0) {
                    printf("connect failed. error is: %d\n", errno);
                    continue;
                }

                if(user_counter >= USER_LIMIT) {
                    const char* info = "too many users\n";
                    printf("%s", info);
                    send(connfd, info, strlen(info), 0);
                    close(connfd);
                    continue;
                }

                user_counter++;
                users[connfd].address = client_address;
                setnoblocking(connfd);
                fds[user_counter].fd = connfd;
                fds[user_counter].events = POLLIN | POLLRDHUP | POLLERR;
                fds[user_counter].revents = 0;
                printf("come a new user<fd: %d>, now have %d users\n", connfd, user_counter);
            } else if(fds[i].revents & POLLERR) {
                printf("get and error from %d\n", fds[i].fd);
                char errors[100];
                memset(errors, '\0', 100);
                socklen_t length = sizeof(errors);
                if(getsockopt(fds[i].fd, SOL_SOCKET, SO_ERROR, &errors, &length) < 0) {
                    printf("get socket option failed\n");
                }
                continue;
            } else if(fds[i].revents & POLLRDHUP) {
                users[fds[i].fd] = users[fds[user_counter].fd]; // 将最大的fd关掉￥
                close(fds[i].fd);
                i--;
                user_counter--;
                printf("a client left. <counter: %d, fd: %d>\n", user_counter, fds[user_counter].fd);
            } else if(fds[i].revents & POLLIN) {
                int connfd = fds[i].fd;
                memset(users[connfd].buf, '\0', BUFFER_SIZE);
                ret = recv(connfd, users[connfd].buf, BUFFER_SIZE, 0);
                printf("got %d bytes data [%s] from client %d\n", ret, users[connfd].buf, connfd);

                if(ret < 0) {
                    if(errno != EAGAIN) {
                        close(connfd);
                        users[fds[i].fd] = users[fds[user_counter].fd];
                        fds[i] = fds[user_counter];
                        i--;
                        user_counter--;
                    }
                } else if(ret == 0) {

                } else {
                    for(int j = 1; j <= user_counter; ++j) {
                        if(fds[j].fd == connfd) {
                            continue;
                        }

                        fds[j].events |= ~POLLIN;
                        fds[j].events |= POLLOUT;
                        users[fds[j].fd].write_buf = users[connfd].buf;
                    }
                    // sleep(1);
                }
            } else if(fds[i].revents & POLLOUT) {
                int connfd = fds[i].fd;
                if (!users[connfd].write_buf) {
                    continue;
                }
                ret = send(connfd, users[connfd].write_buf, strlen(users[connfd].write_buf), 0);
                users[connfd].write_buf = NULL;
                fds[i].events |= ~POLLOUT;
                fds[i].events |= POLLIN;
            }
        }
    }

    delete [] users;
    close(listenfd);

    return 0;
}
