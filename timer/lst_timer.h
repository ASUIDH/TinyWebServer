#ifndef TIMER_H
#define TIMER_H

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <time.h>
#include "../log/log.h"

class util_timer;

struct client_data{
    sockaddr_in address;
    int sockfd;
    util_timer *timer;
};

class util_timer{
public:
    time_t expire;//超时时间
    void (*cb_func)(client_data*);//回调函数
    client_data* user_data;//连接资源
    util_timer* pre;
    util_timer* next;

    util_timer():pre(NULL),next(NULL){}
};
//定时器容器
class sort_timer_lst{
public:
    sort_timer_lst():head(NULL),tail(NULL){}
    ~sort_timer_lst(){
        util_timer *p = head;
        while(p){
            head = p -> next;
            delete p;
            p = head;
        }
    }

    void add_timer(util_timer *timer){
        if(!timer)return;
        if(!head){
            head = tail = timer;
            return;
        }
        if(timer -> expire < head -> expire){
            timer -> next = head;
            head ->pre = timer;
            timer -> pre = NULL;
            head = timer;
            return;
        }
        add_timer(timer,head);
    }

    void adjust_timer(util_timer *timer){
        if(!timer)return;
        util_timer* tmp = timer -> next;
        if(!tmp||timer -> expire < tmp-> expire)return;
        if(timer == head){
            head = head ->next;
            head -> pre = NULL;
            timer -> next = NULL;
            add_timer(timer,head);
        }
        else{
            timer -> pre -> next = timer -> next;
            timer -> next -> pre = timer -> pre;
            add_timer(timer,head);
        }
    }

    void del_timer(util_timer* timer){
        if(!timer)return;
        if(head == timer && tail == timer){
            delete timer;
            head = NULL;
            tail = NULL;
            return;
        }
        if (timer == head){
            head = head->next;
            head->pre = NULL;
            delete timer;
            return;
        }
        if (timer == tail){
            tail = tail->pre;
            tail->next = NULL;
            delete timer;
            return;
        }
        timer->pre->next = timer->next;
        timer->next->pre = timer->pre;
        delete timer;
    }
    void tick(){
        if(!head)return;
        time_t cur = time(NULL);
        util_timer* tmp = head;
        while(tmp){
            if(cur < tmp -> expire)break;
            tmp -> cb_func(tmp -> user_data);
            head = tmp -> next;
            if(head){
                head -> pre = NULL;
            }
            delete tmp;
            tmp = head;
        }
    }

private:
    util_timer *head;
    util_timer *tail;

    void add_timer(util_timer* timer,util_timer* lst_head){
        util_timer *prev = lst_head;
        util_timer *tmp = prev -> next;

        while(tmp){
            if(timer -> expire < tmp -> expire){
                prev -> next = timer;
                timer -> next = tmp;
                tmp-> pre = timer;
                timer -> pre = prev;
                break;
            }
            prev = tmp;
            tmp = tmp -> next;
        }
        if( !tmp ){
            prev->next = timer;
            timer->pre = prev;
            timer->next = NULL;
            tail = timer;
        }
    }

};

class Utils{
public:
    Utils(){}
    ~Utils(){}
    void init(int timeslot);

    //对文件描述符设置非阻塞
    int setnonblocking(int fd);

    //将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
    void addfd(int epollfd, int fd, bool one_shot, int TRIGMode);

    //信号处理函数
    static void sig_handler(int sig);

    //设置信号函数
    void addsig(int sig, void(handler)(int), bool restart = true);

    //定时处理任务，重新定时以不断触发SIGALRM信号
    void timer_handler();

    void show_error(int connfd, const char *info);

public:
    static int *u_pipefd;
    sort_timer_lst m_timer_lst;
    static int u_epollfd;
    int m_TIMESLOT;
};
void cb_func(client_data *user_data);

#endif