#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <pthread.h>
#include "list_timer.h"

#define FD_LIMIT 3
#define TIME_INTERVAL 5 // 闹钟函数定时时间（每 TIME_INTERVAL 秒触发一次）
#define MAX_EVENT_NUMBER 1024

static int pipefd[2];
static sorted_timer_list timer_list;
static int epollfd = 0;

int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);

    return old_option;
}

void add_fd(int epollfd, int fd)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET; // 注册可读事件；设置为边沿触发方式
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

// 信号处理函数
void sig_handler(int sig)
{
    // 保留原来的 errno，在函数最后恢复，以保证函数的可重入性
    int save_errno = errno;
    int msg = sig;

    // 将信号写入管道，以通知主循环
    send(pipefd[1], (char *)&msg, 1, 0);

    errno = save_errno;
}

// 设置信号的处理函数
void add_sig_handler(int sig)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = sig_handler;
    // sigaction 中可以指定收到某个信号时是否可以自动恢复函数执行(即在调用信号处理函数处理完消息后，继续执行原来中断的函数，像什么也没发生一样)。
    // 设置 flag 为 SA_RESTART，表示继续原来的函数。
    sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    // int sigaction(int signum,const struct sigaction *act ,struct sigaction *oldact); 查询或设置信号处理方式
    assert(sigaction(sig, &sa, NULL) != -1);
}

void timer_handler()
{
    timer_list.tick(); // 定时处理任务

    // 因为一次 alarm 系统调用只会引起一次 SIGALRM 信号，所以每次处理完后，都要重新定时。以不断触发 SIGALARM 信号
    // unsigned int alarm(unsigned int seconds); 称为闹钟函数，它可以在进程中设置一个定时。
    // 当定时器指定的时间到时，它向进程发送SIGALRM信号。可以设置忽略或者不捕获此信号，如果采用默认方式其动作是终止调用该alarm函数的进程。
    alarm(TIME_INTERVAL);
}

// 定时器回调函数，它删除非活动连接 socket 上的注册事件，并关闭连接
void timeout_callback_func(client_data* user_data)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, user_data->sockfd, 0);
    assert(user_data);
    close(user_data->sockfd);
    printf("client fd %d closed.\n", user_data->sockfd);
}

int main(int argc, char** argv)
{
    if (argc <= 2) {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }

    const char* ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr); // 点分十进制 -> 网络二进制ip
    address.sin_port = htons(port); // 主机字节序 -> 网络字节序

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd > 0);

    int ret = bind(listenfd, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(listenfd, 5);
    assert(ret != -1);

    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(5);
    assert(epollfd != -1);

    add_fd(epollfd, listenfd);

    // 使用 socketpair 创建一个双向管道。第一个参数 只能使用 UNIX 本地协议族，因为只能在本地使用这个双向管道。 管道内部是字节流，和TCP一致。
    // 它创建的这对文件描述符是可读可写的。这两个文件描述符构成管道的两端。一端写入，另一端就可以读到。读一个空的管道，会被阻塞。
    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, pipefd);
    assert(ret != -1);
    setnonblocking(pipefd[1]);
    add_fd(epollfd, pipefd[0]); // 注册 pipefd[0] 上的可读事件。（我们上面定义的信号处理函数会往 pipefd[1] 中写数据，这里等待可读）

    // 设置信号处理函数（指定要处理那些信号）
    add_sig_handler(SIGALRM);
    add_sig_handler(SIGTERM);
    bool stop_server = false;

    client_data* users = new client_data[FD_LIMIT];
    bool timeout = false;
    alarm(TIME_INTERVAL); // 开启定时

    while(!stop_server) {
        int ready_number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);

        if (ready_number < 0 && errno != EINTR) {
            printf("epoll failure\n");
            break;
        }

        for (int i = 0; i < ready_number; i++) {
            int sockfd = events[i].data.fd;

            // 处理新到的客户连接
            if (sockfd == listenfd) {
                struct sockaddr_in client_address;
                socklen_t client_addr_len = sizeof(client_address);
                int connfd = accept(listenfd, (struct sockaddr*)&client_address, &client_addr_len);
                add_fd(epollfd, connfd);

                users[connfd].address = client_address;
                users[connfd].sockfd = connfd;
                // 创建定时器，并设置其回调函数与超时时间。定时器中绑定用户必须数据
                util_timer* timer = new util_timer;
                timer->user_data = &users[connfd];
                timer->cb_func = timeout_callback_func;
                time_t cur = time(NULL);
                timer->expire = cur + TIME_INTERVAL;
                users[connfd].timer = timer;
                timer_list.add_timer(timer);
                printf("client socket [%d] accepted.\n", connfd);
            }
            // 处理信号
            else if (sockfd == pipefd[0] && events[i].events & EPOLLIN) {
                char signals[1024];
                ret = recv(pipefd[0], signals, sizeof(signals), 0);
                if (ret == -1) {
                    // handle the error
                    continue;
                } else if (ret == 0) {
                    continue;
                } else {
                    // 因为每个信号占 1 字节，所以这里按字节逐个接收信号。
                    for (int j = 0; j < ret; ++j) {
                        switch (signals[j]) {
                            case SIGALRM: {
                                printf("got SIGALARM signal ...\n");
                                timeout = true;
                                break;
                            }
                            case SIGTERM: {
                                printf("got SIGTERM signal. bye-bye ...\n");
                                stop_server = true;
                                // 这里不能 break。否则就跳出 while 了！
                            }
                        }
                    }
                }
            }
            // 处理客户连接上收到的数据
            else if (events[i].events & EPOLLIN) {
                memset(users[sockfd].buf, '\0', BUFFER_SIZE);
                ret = recv(sockfd, users[sockfd].buf, BUFFER_SIZE - 1, 0);
                printf("got %d bytes of client data %s from %d\n", ret, users[sockfd].buf, sockfd);

                util_timer* timer = users[sockfd].timer;
                if (ret < 0) {
                    // 如果发生错误，则关闭连接，移除定时器
                    if (errno != EAGAIN) {
                        timeout_callback_func(&users[sockfd]);
                        if (timer) timer_list.del_timer(timer);
                    }
                } else if (ret == 0) {
                    // 如果对方关闭连接，这里也关闭连接
                    timeout_callback_func(&users[sockfd]);
                    if (timer) timer_list.del_timer(timer);
                } else {
                    if (timer) {
                        time_t cur = time(NULL);
                        timer->expire = cur + 3 * TIME_INTERVAL;
                        printf("[%d] adjust timer now...\n", sockfd);
                        timer_list.adjust_timer(timer);
                    }
                }
            } else {
                // ...
            }
        }

        // 最后处理定时事件，因为 I/O 事件有更高的优先级。 当然，这将会导致定时任务不能精确地按照预期的时间执行。
        if (timeout) {
            timer_handler();
            timeout = false;
        }
    }

    close(listenfd);
    close(pipefd[0]);
    close(pipefd[1]);
    delete [] users;
    return 0;
}