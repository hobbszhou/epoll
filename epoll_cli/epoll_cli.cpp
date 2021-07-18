#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <vector>
#include <string.h>
using namespace std;
int main()
{
    vector<int> v_cli;
    int len;
    struct sockaddr_in address = {0}; //服务器端网络地址结构体
    int result;
    char ch = 'A';
    for (int i = 0; i < 5; ++i)
    {

        int client_sockfd = socket(AF_INET, SOCK_STREAM, 0); //建立客户端socket
        v_cli.push_back(client_sockfd);
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = inet_addr("127.0.0.1");
        address.sin_port = htons(4567);
        len = sizeof(address);
        result = connect(client_sockfd, (struct sockaddr *)&address, len);
        if (result == -1)
        {
            perror("oops: client2");
            exit(1);
        }
        else
        {
            printf("连接服务器成功=%d\n", i);
        }
        char sendbuf[128] = {0};
        snprintf(sendbuf, 128, "info=%s, sockfd=%d", "aaaaa", client_sockfd);
        send(client_sockfd, sendbuf, strlen(sendbuf), 0);
        sleep(1);
        char recvbuf[1024] = {0};
        recv(client_sockfd, recvbuf, sizeof(recvbuf), 0);
        printf("recv info: %s\n", recvbuf);
    }
    int a = 0;
    scanf("%d", &a);
    if (a == 100)
    {
        return 0;
    }
    vector<int>::iterator it = v_cli.begin();
    for (; it != v_cli.end(); ++it)
    {
        printf("close %d\n", *it);

        close(*it);
    }

    return 0;
}