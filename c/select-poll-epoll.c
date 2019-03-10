#include<sys/select.h>
/**
 * select() allow  a program to monitor multiple file descriptors, waiting until one or more of the file descriptors
   become "ready" for some class of I/O operation (e.g., input possible).

 * @param  nfds      nfds is the highest-numbered file descriptor in any of the three sets, plus 1.
 * @param  readfs    [description]
 * @param  writefds  [description]
 * @param  exceptfds exception fds 发生异常的fd。 select 只能处理一种异常：socket 上接收到了带外数据。 —— TCP 用 URG 标识。 urgent
 * @param  timeout   specifies the minimum interval that select() should block waiting for a  file  descriptor  to  become  ready.
 * @return           return the number of file descriptors contained in the three returned descriptor sets 
 *                   (that is, the  total  number  of bits that are set in readfds, writefds, exceptfds) which may be zero if the 
 *                   timeout expires before anything interesting happens. 
 */
int select(int nfds, fd_set* readfs, fd_set* writefds, fd_set* exceptfds, struct timeval* timeout);
	
/**
 * The accept() system call is used with connection-based socket types (SOCK_STREAM, SOCK_SEQPACKET).  
 * It extracts the first connection request on the queue of pending connections for the listening socket, sockfd, 
   creates a new connected socket, and  returns  a new file descriptor referring to that socket.  
 * The newly created socket is not in the listening state. The original socket sockfd is unaffected by this call.
 * 
 * @param  sockfd  [description]
 * @param  addr    [description]
 * @param  addrlen [description]
 * @return         [description]
 */
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

/**
 * poll() performs a similar task to select(2): it waits for one of a set of file descriptors to become ready to perform I/O.
 * @param  fds     [description]
 * @param  nfds    [description]
 * @param  timeout [description]
 * @return         On  success,  a  positive  number is returned;  this is the number of structures which have nonzero revents fields
 *                  (in other words, those descriptors with events or errors reported). 
 */
int poll(struct pollfd *fds, nfds_t nfds, int timeout);
struct pollfd {
   int   fd;         /* file descriptor */
   short events;     /* requested events */
   short revents;    /* returned events revents is  an output parameter, filled by the kernel with the events that actually occurred.*/ 
};

// poll 返回后，只能通过遍历所有已注册的描述符，查找我们关心的就绪的描述符事件。
for (int i = 0; i < nfds; ++i)
{
	if (fds[i].revents & POLLIN) // 读就绪
	{
		int sockfd = fds[i].fd;
		// do sth...
	}
}
// ---------------------------------------------------------------------------------------------------------------

/**
 * The  epoll  API performs a similar task to poll(2): monitoring multiple file descriptors to see if I/O is possible on any of them.
   The epoll API can be used either as an edge-triggered or a level-triggered interface and scales well to large numbers  of  watched
   file descriptors.  The following system calls are provided to create and manage an epoll instance:

*  epoll_create(2)  creates  an  epoll  instance  and  returns  a  file  descriptor  referring to that instance.  (The more recent

*  Interest in particular file descriptors is then registered via epoll_ctl(2).  The set of file descriptors currently  registered
  on an epoll instance is sometimes called an epoll set.

*  epoll_wait(2) waits for I/O events, blocking the calling thread if no events are currently available.
 */
/**
 * The  epoll_wait()  system call waits for events on the epoll(7) instance referred to by the file descriptor epfd.  The memory area
       pointed to by events will contain the events that will be available for the caller. Up to maxevents are returned by epoll_wait().
       The maxevents argument must be greater than zero.
 * @param  epfd      该文件描述符由 epoll_create 创建，它指向当前的内核事件表。
 *                   epoll 将用户关心的文件描述符上的事件放在内核的一个内核事件表中，从而用户就无需像用 select 和 poll 那样，每次调用都重复传入
 *                   文件描述符集或事件集。不过，epoll 需要一个额外的文件描述符 epfp 来唯一标识内核中的这个事件表。
 * @param  events    这个
 * @param  maxevents [description]
 * @param  timeout   specifies the minimum number of milliseconds that epoll_wait() will block.
 * @return           [description]
 */
int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
typedef union epoll_data {
   void    *ptr;
   int      fd;
   uint32_t u32;
   uint64_t u64;
} epoll_data_t;

struct epoll_event {
   uint32_t     events;    /* Epoll events */
   epoll_data_t data;      /* User data variable */
};

// ---------------------------------------------------------------------------------------------------------------

#define MAX_EVENTS 10
struct epoll_event ev, events[MAX_EVENTS];
int listen_sock, conn_sock, nfds, epollfd;

/* Set up listening socket, 'listen_sock' (socket(), bind(), listen()) */

epollfd = epoll_create(10);
if (epollfd == -1) {
   perror("epoll_create");
   exit(EXIT_FAILURE);
}

ev.events = EPOLLIN;
ev.data.fd = listen_sock;
if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listen_sock, &ev) == -1) {
   perror("epoll_ctl: listen_sock");
   exit(EXIT_FAILURE);
}

for (;;) {
   nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1); // nfds 是有就绪事件的 fd 数量。 events 是内核设置好的就绪事件列表
   if (nfds == -1) {
       perror("epoll_pwait");
       exit(EXIT_FAILURE);
   }

   for (n = 0; n < nfds; ++n) {
       if (events[n].data.fd == listen_sock) {
           conn_sock = accept(listen_sock, (struct sockaddr *) &local, &addrlen);
           if (conn_sock == -1) {
               perror("accept");
               exit(EXIT_FAILURE);
           }
           setnonblocking(conn_sock);
           ev.events = EPOLLIN | EPOLLET;
           ev.data.fd = conn_sock;
           if (epoll_ctl(epollfd, EPOLL_CTL_ADD, conn_sock, &ev) == -1) {
               perror("epoll_ctl: conn_sock");
               exit(EXIT_FAILURE);
           }
		} else {
           do_use_fd(events[n].data.fd);
        }
   }
}

// --------------------------------------------------------------------------------------------------------------- //
// --------------------------------------------------------------------------------------------------------------- //

/*
	* 多路复用：多路网络连接复用一个 I/O 线程（在一个 I/O 线程上实现对多个文件描述符的监听）。
	*
	* select poll epoll 都是多路复用的实现，都能同时监听多个文件描述符。
	* 它们等待由 timeout 指定的超时时间，直到一个或多个 fd 上有事件发生，返回值是就绪 fd 的数量。
	* ======
	* select
	* ======
	* 1、select 的参数类型 fd_set 并没有将文件描述符和事件绑定到一起，仅仅是一个文件描述符集合。因此，它需要三组这样的文件描述符集合来分别表示需要监听
	* 的三种事件类型：可读、可写、异常。  这也就使得 select 不能处理更多类型的事件。
	* 2、另一方面，由于内核对传入的 fd_set 集合的在线修改，因此，应用程序下次调用 select 前不得不重置这三个 fd_set 集合。
	* 3、select 的 fd_set 实际上是一个长整型数组，总长度是 1024 位。 所以，它最多能同时监听 1024 个文件描述符。
	*
	* ====
	* poll 
	* ====
	* 1、poll 的参数类型 poll_fd 则将每个文件描述符和它关心的事件定义到一起了。
	* 2、内核每次修改的是 poll_fd 中的 revents 成员，不会操作 events 成员。 因此，下次调用 poll 就不需要重置 poll_fd 类型的那个参数了。
	* 3、poll 能监听的描述符数量是不受限的（严格来说受限于操作系统进程能打开的最大文件描述符数量）。
	*
	* select 和 poll 的一个公共问题是： 用户每次想要取得真正就绪的文件描述符上的事件时，都必须要遍历当前所有监听的 fd. —— 轮询所有。 时间复杂度为 O(N).
	*
	* =====
	* epoll
	* =====
	* 1、epoll 的方式是，在内核中维护一个事件表。并提供了一个独立的系统调用 epoll_ctl 来控制往其中添加、修改、删除事件。 从而，epoll_wait 调用都是直接从这个内核事件表
	* 表中取得用户注册的事件，也就不需要每次都从用户空间读入他们。
	* 2、epoll_wait 的第二个参数 events 仅用来返回就绪的事件，所以，用户就可以在 O(1) 的时间内找到就绪的事件了。
	* 3、epoll 和 poll 都能监听系统允许打开的最大文件描述符数量的fd。
	* 4、select 和 poll 都只能工作在相对低效的 LT 模式，而 epoll 可以工作在 ET 高效模式。
	* 5、epoll 还支持 EPOLLONESHOT 事件，可以使得 fd 上的事件真正只触发一次。（一次触发后，后续不再触发；直到事件被用户程序处理完，并修改 fd 的状态，内核才会重新照看它）。
	*
	* 从实现原理上来说，select 和 poll 都是采用轮询的方式，即每次都要遍历所有已注册的 fd。 因此，检测就绪 fd 的时间复杂度是 O(N) 
	* 而 epoll_wait 则是采用回调的方式，内核检测到就绪的 fd 时，将触发回调方法，将该 fd 对应的事件插入内核就绪事件队列。所以，用户无需轮询来查找就绪的 fd 事件。 O(1)。
	* 
	* 当然，当就绪事件足够多的时候，也就是 epoll 的 events 中有很多的就绪事件，那么 epoll 的效率也就不会比 poll 高多少了。—— 因为工作量相当呀~
	*
	* -----------------------------------------------------------------------------------------------------------------------------------
	* LT vs ET
	* LT(Level Trigger), 电平触发。 这是默认的模式。
	* 	采用 LT 模式时，当 epoll_wait 上检测到 fd 上有就绪事件并通知应用程序后，应用程序可以不立即处理该事件。这样，当应用程序下次调用 epoll_wait 时，epoll_wait 还会
	* 	再次向应用程序通知那个 fd 上的就绪事件。 直到该事件被处理。
	* ET(Edge Trigger)，边缘触发。 使用 ET 时，就绪事件只会被通知一次，所以应用程序应该立即处理。 但是一个fd，依然有可能多次触发多次的事件，比如上面的读就绪后，被应用程序
	* 读取，然后应用程序处理这些数据；这个过程中，改 fd 上又有了可读的就绪事件，它就会被分配给别的线程或进程处理。此时就会有多个线程|进程在处理同一个 socket。这显然不是我
	* 们想要的啦。于是有了 EPOLLONESHOT。 注册了改事件的 fd|socket 只会被一个进程处理。 另外，处理完后，必须重置该 fd 上的事件哦！否则操作系统不知道你有没有处理完，就不会
	* 再通知上面的就绪事件了。
	*
	*
	* -----------------------------------------------------------------------------------------------------------------------------------
	* 关于打开的文件描述符数量
	* ----------------------
	* ** /proc/sys/fs/file-max 是系统对所有进程能打开的 fd 的一个总的限制，它是系统根据内存大小给出的建议值，大约是内存的 10%。
	* 	man proc 中可以找到： This  file defines a system-wide limit on the number of open files for all processes.
	* 	
	* ** ulimit	它描述的是当前 shell 环境对它多能打开的 fd 数量限制，或者是 shell 启动的那些进程能打开的 fd 数量。 —— 用 ulimit -n 查看。
	* 	Provides control over the resources available to the shell and to processes started by it, on systems that allow such  control. (man ulimit)
	* 
	* 1、/proc/sys/fs/file-max 限制不了 /etc/security/limits.conf
	* 2、只有root用户才有权限修改/etc/security/limits.conf
	*   对于非root用户， /etc/security/limits.conf会限制ulimit -n，但是限制不了root用户
	*   对于非root用户，ulimit -n 只能越设置越小(多次调用 unlimit -n 设置允许打开的描述符)，root用户则无限制
	*   任何用户对ulimit -n的修改只在当前环境有效，退出后失效，重新登录新来后，ulimit -n由limits.conf决定
	*   如果limits.conf没有做设定，则默认值是1024
	*   当前环境的用户所有进程能打开的最大问价数量由ulimit -n决定
 */
