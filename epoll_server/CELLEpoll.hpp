
#ifndef _CELL_EPOLL_HPP_
#define _CELL_EPOLL_HPP_

#ifdef __linux__
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/epoll.h> //epoll头文件
#include <stdio.h>
#include <vector>
#include <thread>
#include <sys/time.h>
#include <algorithm>
#include <functional>
#define SOCKET int
#define INVALID_SOCKET (SOCKET)(~0)
#define SOCKET_ERROR (-1)
#define EPOLL_ERROR (-1)

// 把常用的epoll使用到的函数封装到下面的这个类中
class CELLEpoll
{
private:
    /* data */
public:
    //CELLEpoll(/* args */){};
    ~CELLEpoll()
    {
        destroy();
    }

public:
    int create(int nMaxEvents)
    {
        if (_epfd > 0)
        {
            //warning
            destroy();
            return -1;
        }
        _epfd = epoll_create(nMaxEvents);
        if (_epfd == EPOLL_ERROR)
        {
            // sreerror(errno);  // 输出错误码信息
            perror("epoll create\n"); // 往标准错误输出， 相当于往 stderr描述符中写东西
            return _epfd;
        }
        _pEvents = new epoll_event[nMaxEvents];
        _nMaxEvents = nMaxEvents;
        return _epfd;
    }
    int ctl(int op, SOCKET sockfd, uint32_t events)
    {

        epoll_event ev;
        // 事件类型
        ev.events = events; //关心可读事件
        ev.data.fd = sockfd;
        // 向epoll 对象注册需要管理、监听的socket文件描述符
        // 并且说明关心的事件
        int ret = epoll_ctl(_epfd, op, sockfd, &ev);
        if (ret == EPOLL_ERROR) // 现在这个epfd就可以来进行管理我们的_sock了， 返回值小于0的话，代表失败
        {
            perror("error: epoll_ctl()\n");
        }
        return ret;
    }
    int wait(const int timeout)
    {
        int ret = epoll_wait(_epfd, _pEvents, _nMaxEvents, timeout);
        if (ret == EPOLL_ERROR)
        {
            perror("epoll_ctl");
            return ret;
        }
        return ret;
    }
    epoll_event *events() // 专门用于返回事件的接口
    {
        return _pEvents;
    }
    void destroy()
    {

        if (_epfd > 0)
        {
            close(_epfd);
            _epfd = -1;
        }

        if (_pEvents)
        {
            delete[] _pEvents;
            _pEvents = nullptr;
        }
    }

private:
    epoll_event *_pEvents = nullptr;
    int _epfd = -1;
    int _nMaxEvents = 1;
};

#endif
#endif
