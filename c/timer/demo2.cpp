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
#include "time_wheel_timer.h"

int main (int argc, char** argv)
{
    time_t cur = time(NULL);
    time_wheel* tw = new time_wheel();
    tw->add_timer(cur + 15);
    tw->add_timer(cur + 80);
    tw->add_timer(cur + 125);
    tw->add_timer(cur + 25);
    tw->add_timer(cur + 6);
    tw->add_timer(cur + 0.1);;

    tw->to_string();
}
