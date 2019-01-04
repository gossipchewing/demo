#ifndef TIME_WHEEL_TIMER
#define TIME_WHEEL_TIMER

#include <time.h>
#include <netinet/in.h>
#include <stdio.h>

#define BUFFER_SIZE 64

class tw_timer; // 前向声明

// 用户数据结构
struct client_data
{
    sockaddr_in address; // 客户端 socket 地址
    int sockfd; // socket fd
    char buf[BUFFER_SIZE]; // 读缓存
    tw_timer* timer; // 定时器
};

// 时间轮定时器。 超时时间由（定时器所在的槽slot + 圈数）决定
class tw_timer
{
    public:
        tw_timer(int rot, int ts) : prev(NULL), next(NULL), rotation(rot), time_slot(ts) {}

    public:
        void (*cb_func)(client_data *data); // 任务回调函数
        client_data* user_data; // 任务回调函数处理的客户数据，由定时器的执行者传递给回调函数
        // 初始化时，变量的使用顺序要和这里声明出现的顺序一致，否则编译时会有警告
        tw_timer* prev;
        tw_timer* next;
        int rotation; // 记录定时器在时间轮转多少圈后生效（这里只有一个轮，所以要记录圈数）
        int time_slot; // 表示定时器属于时间轮第 time_slot 个槽的链表中。
};

class time_wheel
{
    public:
        time_wheel() : cur_slot(0)
        {
            for (int i = 0; i < N; i++) {
                slots[i] = NULL; // 初始化每个槽的头结点
            }
        }

        ~time_wheel()
        {
            // 销毁所有槽中的定时器链表
            for (int i = 0; i < N; i++) {
                tw_timer* tmp = slots[i];
                while (tmp) {
                    slots[i] = tmp->next;
                    delete tmp;
                    tmp = slots[i];
                }
            }
        }

        tw_timer* add_timer(int timeout)
        {
            if (timeout < 0) return NULL;

            int ticks = 1;
            if (timeout > SI) {
                ticks = timeout / SI; // 定时器将在 ticks 个 SI 后被触发（不足一个 SI 的为 1）
            }

            int rotation = ticks / N;
            int tm_slot = (cur_slot + ticks) % N;
            tw_timer* timer = new tw_timer(rotation, tm_slot);

            if (!slots[tm_slot]) {
                slots[tm_slot] = timer; // tm_slot 处的定时器链表为空
            } else {
                timer->next = slots[tm_slot];
                slots[tm_slot]->prev = timer;
                slots[tm_slot] = timer;
            }
            printf("add timer, rotation is %d, timer slot is %d, current slot is %d\n", rotation, tm_slot, cur_slot);

            return timer;
        }

        void del_timer(tw_timer* timer)
        {
            if (!timer) return;

            tw_timer* headSlot = slots[timer->time_slot];

            if (timer == headSlot) {
                headSlot = timer->next;
                if (headSlot) headSlot->prev = NULL;
            } else {
                timer->prev->next = timer->next;
                if (timer->next) timer->next->prev = timer->prev;
            }

            delete timer;
        }

        // 每次被调用都往前移动一个 slot。 SIGALARM 信号每次被触发，就在信号处理函数中执行一次 tick 函数，来处理链表上到期的定时任务
        void tick()
        {
            tw_timer* tmp = slots[cur_slot];
            printf("tick... current slot is %d\n", cur_slot);

            // 遍历当前槽的定时器链表
            while (tmp) {
                if (tmp->rotation > 0) { // 时间未到，这一轮不用处理
                    tmp->rotation--;
                    tmp = tmp->next;
                    continue;
                }

                // 定时器到期，处理事件。 并删除定时器
                tmp->cb_func(tmp->user_data);

                if (tmp == slots[cur_slot]) { // 如果当前是头结点
                    slots[cur_slot] = tmp->next;
                    if (slots[cur_slot]) slots[cur_slot]->prev = NULL;
                    delete tmp;
                    tmp = slots[cur_slot];
                } else {
                    tmp->prev->next = tmp->next;
                    if (tmp->next) tmp->next->prev = tmp->prev;
                    tw_timer* tmp2 = tmp;
                    delete tmp;
                    tmp = tmp2;
                }
            }

            cur_slot = ++cur_slot % N; // 更新时间轮的当前槽，以反映时间轮的转动
        }

        void to_string()
        {
            for (int i = 0; i < N; i++) {
                tw_timer* tmp = slots[i];
                printf("slot[%d]: ", i);
                while (tmp) {
                    printf("<%d, %d>\t", tmp->rotation, tmp->time_slot);
                    tmp = tmp->next;
                }
                printf("\n");
            }
        }

    private:
        static const int N = 60; // 时间轮上槽的数量
        static const int SI; // 每个槽代表的时间周期(slot interval)
        tw_timer* slots[N]; // 时间轮（其中的每个槽指向一个定时器链表，链表无序）
        int cur_slot; // 时间轮的当前槽
};

#endif
