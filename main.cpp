
#include "CELLEpoll.hpp"
using namespace std;
std::vector<SOCKET> g_clients;

bool g_bRun = true;
void cmdThread() // 命令线程
{
    while (true)
    {
        char cmdBuf[256] = {};
        scanf("%s", cmdBuf);
        if (0 == strcmp(cmdBuf, "exit"))
        {
            g_bRun = false;
            printf("退出cmdThread线程\n");
            break;
        }
        else
        {
            printf("不支持的命令。\n");
        }
    }
}
#if 0
int recvData(SOCKET cSock)
{
    char szRecv[4096] = {};

    int nLen = (int)recv(cSock, szRecv, 4096, 0);
    if (nLen <= 0)
    {
        printf("客户端<Socket=%d>已退出， 任务结束\n", cSock);
        return -1;
    }
    printf("info=%s\n", szRecv);

    return 0;
}
#endif

char g_szBuff[256] = {0};
int g_nLen = 0;
int readData(SOCKET cSock)
{
    g_nLen = (int)recv(cSock, g_szBuff, 256, 0);
    if (g_nLen <= 0)
    {
        return -1;
    }
    printf("info=%s\n", g_szBuff);
    return g_nLen;
}
int writeData(SOCKET cSock)
{
    if (g_nLen > 0)
    {
        int nLen = (int)send(cSock, g_szBuff, g_nLen, 0);
        g_nLen = 0;
        return nLen;
    }
    return 1;
}

int clientLeave(SOCKET cSock)
{
    printf("客户端<socket=%d>已退出，任务结束\n", cSock); // 将socket从 sokcet里面进行移除
    close(cSock);                                         // 关闭客户端
    auto iter = std::find(g_clients.begin(), g_clients.end(), cSock);
    if (iter != g_clients.end())
    {
        g_clients.erase(iter); // 从vector中进行移除
    }

    return 0;
}

int main()
{
    std::thread t1(cmdThread); // 命令线程
    t1.detach();

    SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in _sin = {};
    _sin.sin_family = AF_INET;
    _sin.sin_port = htons(4567);
    _sin.sin_addr.s_addr = INADDR_ANY;

    // 2
    if (SOCKET_ERROR == bind(_sock, (sockaddr *)&_sin, sizeof(_sin)))
    {
        printf("错误，绑定网络端口失败...\n");
    }
    else
    {
        printf("绑定网络端口成功\n");
    }
    // 3 listen 监听网络端口
    if (SOCKET_ERROR == listen(_sock, 5))
    {
        printf("错误，监听网络端口失败...\n");
    }
    else
    {
        printf("监听网络端口成功....\n");
    }
    // linux 2.6.8 后size就没有作用了
    // 由epoll系统动态管理, linux最大的为filemax
    // 通过cat /proc/sys/fs/file-max来查询

    const int maxClient = 256; // epfd
    CELLEpoll ep;
    int epfd = ep.create(maxClient);
    // int ctl(int op, SOCKET sockfd, uint32_t events)
    ep.ctl(EPOLL_CTL_ADD, _sock, EPOLLIN);

    int msgCount = 0;
    int cCount = 0;
    //epoll_event events[maxClient] = {}; // 创建一个空的事件数组，用来检测接收到的事件就行了
    while (g_bRun)
    {

        // epfd epoll, 第一个参数是创建的epoll对象，   events 第二个参数是接收事件数组，用于接收检测到的网络时间的数组
        // maxevents 第三个参数是最多接受多少个事件，不能超过我们的数组大小，要不然会越。
        // 第四个参数是 超时时间， timeout:
        //                                t = 0, 是一直等还是立即返回， 0代表立即返回，
        //                                t = -1会阻塞，一直等待有事件发生 ,直到有时间发生才会返回
        //                                t > 0, 大于0的话就是等待多少毫秒
        int n = ep.wait(1); //等待事件的发生， 不会阻塞的， 注意这里的events是一个空的
        if (n < 0)          // n < 0的话，代表出错了;   在超时时间内，如果没出错，也没事件发生，那就是返回0 的
        {
            printf("epoll_ait ret = %d\n", n);
            break;
        }
        auto events = ep.events();
        // 如果大于0的话，就表示有时间发生了，相应的去处理时间就行
        for (int i = 0; i < n; ++i)
        {
            if (events[i].data.fd == _sock) //当有新客户端加入的时候
            {

                sockaddr_in clientAddr = {0};
                int nAddrLen = sizeof(sockaddr_in);
                SOCKET _cSock = INVALID_SOCKET;
                _cSock = accept(_sock, (sockaddr *)&clientAddr, (socklen_t *)&nAddrLen);
                if (INVALID_SOCKET == _cSock)
                {
                    printf("错误，接收到的无效客户端SOCKET ....\n");
                }
                else // epoll 默认是LT模式，也就是水平触发， 会一直进行触发
                {
                    g_clients.push_back(_cSock); // 把新加入的客户端存起来
                    ep.ctl(EPOLL_CTL_ADD, _cSock, EPOLLIN);
                    //ep.ctl(EPOLL_CTL_ADD, _cSock, EPOLLIN|EPOLLOUT);// 按位运算符或运算
                    printf("新客户端加入: socket=%d, IP=%s \n", (int)_cSock, inet_ntoa(clientAddr.sin_addr));
                }
                continue; // 如果是新客户端连进来，就不能走到下面的读数据的代码
            }
            // 当前socket有数据可以读. 也有可能是出现了错误，出现了错误也是有数据可以读
            if (events[i].events & EPOLLIN) // 这时候我们开始打印客户端发过来的数据
            {
                ++msgCount;
                //sleep(5);
                //printf("其他\n");
                printf("EPOLLIN | %d\n", msgCount);
                auto cSock = events[i].data.fd;
                int ret = readData(cSock);
                if (ret <= 0) // 如果客户端出现问题，将客户端进行移除
                {
                    clientLeave(cSock);
                    continue; // 如果客户端出问题了，不应该执行cell_epoll_ctl，所以得用continue进行跳过
                }
                else // 否则打印接收到了数据
                {
                    printf("收到客户端的数据, 第%d条消息, socket=%d, len=%d \n", msgCount, (int)events[i].data.fd, ret);
                }
                //ep.ctl(EPOLL_CTL_ADD, cSock, EPOLLOUT); // 当收到数据了之后，把关注点改为可写
                ep.ctl(EPOLL_CTL_MOD, cSock, EPOLLOUT); // 当收到数据了之后，把关注点改为可写，
                                                        // 修改事件一定得使用EPOLL_CTL_MOD，上面得那行代码那样的写就是错误的，上面的那一行是添加事件
                //ep.ctl(EPOLL_CTL_MOD, _cSock, EPOLLIN|EPOLLOUT); // 这样写的话，既关注读，也关注写，同时进行检测两个
            }
            // 当前 socket 是否可写数据
            if (events[i].events & EPOLLOUT)
            {
                printf("EPOLLOUT | %d\n", msgCount);
                auto cSock = events[i].data.fd;
                int ret = writeData(cSock);
                if (ret <= 0)
                {
                    clientLeave(cSock); //  ep.ctl(EPOLL_CTL_DEK, cSock, 0);  这种的写法可以代替 clientLeave， 只是将客户端从epoll里面移除掉了，
                                        //  但是保留这个套接字仍处于连接状态，不进行关闭
                    continue;           // 如果客户端出问题了，不应该执行cell_epoll_ctl，所以得用continue进行跳过
                }
                ep.ctl(EPOLL_CTL_MOD, cSock, EPOLLIN); // 当我们写入一条数据之后，把关注点改为可读
#if 0
                if (msgCount < 5) // 如果小于5次，我们还继续监听客户端消息，否则我们呢就不监听了，将他从epoll中进行移除掉了
                {
                    ep.ctl(EPOLL_CTL_MOD, cSock, EPOLLIN); // 当我们写入一条数据之后，把关注点改为可读
                }
                else
                {
                    ep.ctl(EPOLL_CTL_DEL, cSock, 0); // 从epoll中进行移除
                }
#endif
            }
            // sleep(1);
#if 0
            if (events[i].events & EPOLLERR) // 当我们写逻辑的时候，还加这个判断，会导致开销增加的
            {
                auto cSock = events[i].data.fd;
                printf("EPOLLER: id=%d, socket=%d\n", msgCount, cSock);
            }
            if (events[i].events & EPOLLHUP) // 对应的文件描述符被挂断
            {
                auto cSock = events[i].data.fd;
                printf("EPOLLLHUP: id=%d, socket=%d \n", msgCount, cSock);
            }
#endif
        }
    }
    for (auto client : g_clients)
    {
        close(client); // 当我们关闭socket的时候，是会自动的从 epoll中进行移除掉的
    }

    //close(epfd);  // 关闭epfd epoll对象
    ep.destroy();
    close(_sock); // 关闭sock 文件描述符
    return 0;
}