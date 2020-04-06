






#include <iostream>
#include <stdio.h>
#include <stdint.h>
using namespace std;

// linux 
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <errno.h>

#define DEBUG 1
#define INFO 2
#define ERR 2

#define log_err(format, ...) \
    printf(format "\n", ##__VA_ARGS__);

#define log_debug(format, ...) \
    printf(format "\n", ##__VA_ARGS__);



#define MAX_EVENTS 1000


void EventAdd(int epollFd, int iofd, int events, void *data)
{
    struct epoll_event epv = {0, {0}};
    
    epv.events = EPOLLIN | EPOLLET;
    epv.data.fd = iofd;

    if(epoll_ctl(epollFd, EPOLL_CTL_ADD, iofd, &epv) < 0)
    {
        log_err("add sock to event fail");
    }
    else 
    {
        log_debug("add sock ok");
    }
}

void EventDel(int epollFd, int iofd, void* data)
{
    struct epoll_event epv = {0, {0}};

    epoll_ctl(epollFd, EPOLL_CTL_DEL, iofd, &epv);

}

int acceptFd;
int g_epollFD;

int init_socket(int epollFd, int port)
{
    int listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenFd < 0)
    {
        log_debug("Create socket fail");
        return -1;
    }
    fcntl(listenFd, F_SETFL, 0, O_NONBLOCK);

    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(8888);
    int iRet = bind(listenFd, (const sockaddr*)&sin, sizeof(sin));
    if(iRet < 0)
    {
        log_err("bind fail");
        close(listenFd);
        return -1;
    }

    listen(listenFd, 5);

    acceptFd = listenFd;

    EventAdd(epollFd, listenFd, 0, NULL);
    return 1;
}

void AcceptConn(int fd)
{
    struct sockaddr_in sin;
    socklen_t len = sizeof(struct sockaddr_in);
    int clientFD, i;
    if((clientFD = accept(fd, (struct sockaddr*)&sin, &len)) == -1)
    {
        log_err("accept fail");
        return;
    }
    int iRet = fcntl(clientFD, F_SETFL, O_NONBLOCK);
    if(iRet < 0)
    {
        close(clientFD);
        return;
    }
    EventAdd(g_epollFD, clientFD, 0, NULL);
    log_debug("accept a client");
}

void ReadData(struct epoll_event* ev)
{
    if(ev->data.fd == acceptFd)
    {
        log_debug("accept one client");
        AcceptConn(ev->data.fd);
        return;
    }
    int ret = 0;
    int index = 0;
    char buff[128];
    buff[127] = '\0';

    while((ret = read(ev->data.fd, buff + index, 127 - index)) < 127)
    {
        if(errno == EAGAIN)
        {
            break;
        }
        index += ret;
    }
    printf("%s", buff);
    return;
}

void WriteData(int fd, char* buf, int size)
{
    int ret = 0;
    int index = 0;
    while((ret = write(fd, buf + index, size - index)) < size)
    {
        if(errno == EAGAIN)
            break;
        index += ret;
    }
}

int main(int argc , char** argv)
{
    int32_t epollFd[MAX_EVENTS];
    int32_t efd = epoll_create(MAX_EVENTS);
    if(efd <= 0) 
        log_err("create epoll failed\n");

    g_epollFD = efd;

    // 创建 socket
    int iRet = init_socket(efd, 111);

    struct epoll_event  events[MAX_EVENTS];
    log_debug("Server running\n");

    while(1){
        long now = time(NULL);
        #if 0
        for (int32_t i = 0; i < count; i++)
        {
            /* code */
        }
        #endif

        int fds = epoll_wait(efd, events, MAX_EVENTS, 1000);
        if(fds < 0)
        {
            log_err("epoll_wait error, exit \n");
            break;
        }
        for (int i = 0; i < fds; ++i)
        {
            if(events[i].events & EPOLLIN)  // read event
            {
                ReadData(&events[i]);
            }
            if(events[i].events & EPOLLOUT) // write event
            {
                WriteData(events[i].data.fd, NULL, 0);
            }
        }
    }
    return 0;
}






