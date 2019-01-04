#ifndef LIST_TIMER
#define LIST_TIMER

#include <time.h>

#define BUFFER_SIZE 64

class util_timer; // 前向声明

// 用户数据结构
struct client_data
{
    sockaddr_in address; // 客户端 socket 地址
    int sockfd; // socket fd
    char buf[BUFFER_SIZE]; // 读缓存
    util_timer* timer; // 定时器
};

class util_timer
{
    public:
        util_timer() : prev(NULL), next(NULL) {}

    public:
        time_t expire; // 任务的超时时间
        void (*cb_func)(client_data *data); // 任务回调函数
        client_data* user_data; // 任务回调函数处理的客户数据，由定时器的执行者传递给回调函数
        util_timer* prev;
        util_timer* next;
};

class sorted_timer_list
{
    public:
        sorted_timer_list() : head(NULL), tail(NULL) {}

        ~sorted_timer_list()
        {
            util_timer* tmp = head;

            while (tmp) {
                head = tmp->next;
                delete tmp;
                tmp = head;
            }
        }

        void add_timer(util_timer* timer)
        {
            if (!timer) return;

            if (!head) {
                head = tail = timer;
                return;
            }

            util_timer* tmp = head;
            util_timer* prev = NULL;

            while(tmp && timer->expire > tmp->expire) {
                prev = tmp;
                tmp = tmp->next;
            }

            if (!tmp) {
                timer->prev = prev;
                timer->next = NULL;
                prev->next = timer;
                tail = timer;

                return;
            }

            // insert before tmp-node
            timer->prev = tmp->prev;
            timer->next = tmp;
            if (tmp->prev != NULL) tmp->prev->next = timer; // #1
            tmp->prev = timer; // #2
            if (tmp == head) head = timer;
        }

        // 外部修改了某个定时器的超时时间后，重新把它丢到定时器列表中。
        void adjust_timer(util_timer* timer)
        {
            util_timer* tmp = timer;
            del_timer(timer);
            add_timer(tmp);
        }


        void del_timer(util_timer* timer)
        {
            if (!timer) return;

            if ((head == timer) && (tail == timer)) {
                head = tail = NULL;
                delete timer;
                return;
            }

            if (head == timer) {
                head = head->next;
                head->prev = NULL;
                delete timer;
                return;
            }

            if (tail == timer) {
                tail = tail->prev;
                tail->next = NULL;
                delete timer;
                return;
            }

            timer->next->prev = timer->prev;
            timer->prev->next = timer->next;
            delete timer;
            return;
        }

        // SIGALARM 信号每次被触发，就在信号处理函数中执行一次 tick 函数，来处理链表上到期的定时任务
        void tick()
        {
            if (!head) return;

            printf("timer tick\n");
            time_t cur = time(NULL);
            util_timer* tmp = head;

            // 处理所有到期的定时任务
            while(tmp) {
                if (cur < tmp->expire) break;

                tmp->cb_func(tmp->user_data);

                head = tmp->next;
                if (head) head->prev = NULL;
                delete tmp;
                tmp = head;
            }
        }

        void to_string()
        {
            util_timer* tmp = head;

            while (tmp) {
                printf("%ld\t", tmp->expire);
                tmp = tmp->next;
            }
            printf("\n");

            tmp = tail;
            while (tmp) {
                printf("%ld\t", tmp->expire);
                tmp = tmp->prev;
            }
            printf("\n");
        }

    private:
        util_timer* head;
        util_timer* tail;
};

#endif
