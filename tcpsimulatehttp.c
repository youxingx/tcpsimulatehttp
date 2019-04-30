#include <string.h>
#include <sys/un.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <time.h>
#include <sys/epoll.h>

// EACCES Permission to create a socket of the specified type and/or protocol is denied.

// EAFNOSUPPORT
//       The implementation does not support the specified address family.

// EINVAL Unknown protocol, or protocol family not available.

// EINVAL Invalid flags in type.

// EMFILE Process file table overflow.

// ENFILE The system limit on the total number of open files has been reached.

// ENOBUFS or ENOMEM
//       Insufficient memory is available.  The socket cannot be created until sufficient resources are freed.

// EPROTONOSUPPORT

// #define COUNT               20000//同时在线人数５
// #define THREAD_COUNT        10000
#define THREAD_COUNT        10000
#define CONNECT_TIME_OUT    10000
#define EVENTS              1

extern int errno;

int success_count = 0;
int failed_count = 0;
pthread_mutex_t success_lock;
pthread_mutex_t failed_lock;

char *http_header = "GET / HTTP/1.1\r\n"
                                        "Host: www.idoushuo.com\r\n"
                                        "Connection: keep-alive\r\ni"
                                        "Accept: */*\r\n"
                                        "Connection: close\r\n"
                                        "\r\n";

int print_error()
{
    printf("======================error code=====================\n");
    printf("EACCES:%d\n", EACCES);
    printf("EAFNOSUPPORT:%d\n", EAFNOSUPPORT);
    printf("EINVAL:%d\n", EINVAL);
    printf("EMFILE:%d\n", EMFILE);
    printf("ENOBUFS:%d\n", ENOBUFS);
    printf("ENOMEM:%d\n", ENOMEM);
    printf("EPROTONOSUPPORT:%d\n", EPROTONOSUPPORT);
    printf("======================error code=====================\n");
    printf("\n\n\n\n");
    return 0;
}

void* print_message_function( void *ptr ) {
    // sleep(1);
    // int i = 0;
    // for (i; i<5; i++) {
    //     printf("%s:%d\n", (char *)ptr, i);
    // }
    // int i = *(int *)ptr;
    // printf("%d", *(int *)ptr);
    // int retval = i;
    // printf("hello\n");
    // return (void *)&retval;
    // printf("\n\n\n");

    // int val = 1;

    char buff[256] = "GET / HTTP/1.1\r\nHost: 10.0.0.30\r\nConnection: close\r\n\r\n";

    int j = 0;
    for(j=0;j<10;j++){

        struct epoll_event ev, ev_ret[EVENTS];

        struct sockaddr_in servaddr;
        int sockfd;
        int epfd;
        int nfds;
        /* [socket] included by <sys/socket.h> */
        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            /* [perror] included by <stdio.h> */
            perror("socket error");
            exit(-1);
        }

        bzero(&servaddr, sizeof(servaddr));
        /* [htons] included by <arpa/inet.h> */
        servaddr.sin_port = htons(8091);

        /* [AF_INET] included by <bits/socket.h>, but <sys/socket.h> inlude it */
        servaddr.sin_family = AF_INET;

        if ((inet_pton(AF_INET, "10.0.0.220", &servaddr.sin_addr)) < 0) {
            printf("inet_pton error\n");
            exit(-1);
        }

        //设置epoll监测socketfd
        epfd = epoll_create(1);
        if(epfd < 0){
            printf("thread:%d, count:%d\n", *(int *)ptr, j);
            perror("epfd");
            exit(-1);
        }

        memset(&ev, 0, sizeof(ev));
        // ev.events = EPOLLIN;
        ev.events = EPOLLOUT;
        ev.data.fd = sockfd;

        if(epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev) != 0){
            perror("epoll_ctl");
            return NULL;
            // exit(-1);
        }

        unsigned long ul = 1;
        ioctl(sockfd, FIONBIO, &ul); //设置为非阻塞模式
        bool ret = false;

        if(connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0){
            if(errno == EINPROGRESS){
                // printf("before epoll_wait\n");
                nfds = epoll_wait(epfd, ev_ret, EVENTS, CONNECT_TIME_OUT);//timeout is 5 sec
                if(nfds < 0){
                    perror("epoll_wait error\n");
                    ret = false;
                } else if(nfds == 0){
                    unsigned int port = 0;
                    struct sockaddr_in localaddr;
                    //一定要给出结构体大小，要不然获取到的端口号可能是0
                    socklen_t len = sizeof(localaddr);
                    //fd是创建的套接字
                    int re = getsockname(sockfd, (struct sockaddr*)&localaddr, &len);

                    if(re != 0)
                    {
                        perror("getsockname");
                    }
                    else
                    {
                        // perror("getsockname");
                        port = ntohs(localaddr.sin_port);
                        // printf("port: %d\n", ntohs(localaddr.sin_port));
                    }
                    // printf("connect timeout, thread:%d, count:%d\n", *(int *)ptr, j);
                    pthread_mutex_lock(&failed_lock);
                    failed_count++;
                    printf("connect timeout! thread:%d, count:%d, port:%d ,success_count:%d, errno:%d\n", *(int *)ptr, j, port, success_count, errno);
                    // printf("connect timeout\n");
                    pthread_mutex_unlock(&failed_lock);
                    ret = false;
                } else {
                    ret = true;
                }
            } else {
                perror("connect error\n");
                ret = false;
                return NULL;
                // return -1;
                // exit(-1);
            }
        }

        // sleep(1);

        // int error=-1, len;
        // len = sizeof(int);
        // struct timeval tm;
        // fd_set set;
        // unsigned long ul = 1;
        // ioctl(sockfd, FIONBIO, &ul); //设置为非阻塞模式
        // bool ret = false;
        // if( connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
        // {
        //     tm.tv_sec = CONNECT_TIME_OUT;
        //     tm.tv_usec = 0;
        //     FD_ZERO(&set);
        //     FD_SET(sockfd, &set);
        //     if( select(sockfd+1, NULL, &set, NULL, &tm) > 0)
        //     {
        //         // printf("select ok\n");
        //         getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, (socklen_t *)&len);
        //         if(error == 0) { ret = true; }
        //         else { ret = false; }
        //     } else {
        //         perror("connect error");
        //         // printf("???");
        //         ret = false;
        //     } 
        // } else {
        //     ret = true;
        // }

        // ul = 0;
        // ioctl(sockfd, FIONBIO, &ul); //设置为阻塞模式
        if(!ret) {
            // close( sockfd );
            // pthread_mutex_lock(&success_lock);
            // printf("thread:%d, count:%d, success_count:%d, errno:%d\n", *(int *)ptr, j, success_count, errno);
            // printf("connect timeout\n");
            // pthread_mutex_unlock(&success_lock);
            // exit(-1);
            // return;
        } else {
            //获取线程锁
            pthread_mutex_lock(&success_lock);
            success_count++;
            //释放线程锁
            pthread_mutex_unlock(&success_lock);
        }

        /* [connect] included by <sys/socket.h> */
        // if ((connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) < 0) {
        //     //获取线程锁
        //     pthread_mutex_lock(&success_lock);
        //     printf("thread:%d, count:%d, success_count, errno:%d\n", *(int *)ptr, j, success_count, errno);
        //     perror("connect error");
        //     //释放线程锁
        //     pthread_mutex_unlock(&success_lock);
        //     exit(-1);
        // } else {
        //     //获取线程锁
        //     pthread_mutex_lock(&success_lock);
        //     success_count++;
        //     //释放线程锁
        //     pthread_mutex_unlock(&success_lock);
        // }
        
        // char recvline[1024];
        // int k=0;
        // for(k=0; k<1024; k++){
        //     recvline[k] = '\0';
        // }

        // /* [write] and [read] included by <unistd.h> */
        // if ((write(sockfd, buff, strlen(buff))) < 0) {
        //     perror("write error");  
        //     exit(-1);
        // }
        // while (read(sockfd, recvline, sizeof(recvline))) {
        //     // printf("%s", recvline);
        // }

        // if(epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, &ev) < 0)
        // {
        //     // printf("删除事件失败");
        //     perror("del failed");
        //     return NULL;
        // }
        close(sockfd);
        close(epfd);
        // printf("\n");
    }

    return NULL;
}

int main()
{
    // print_error();

    if (pthread_mutex_init(&success_lock, NULL) != 0) {  
        printf("mutex init failed\n");
        return -1;
    }  

    if (pthread_mutex_init(&failed_lock, NULL) != 0) {  
        printf("mutex init failed\n");
        return -1;
    } 

    // int sockfd[COUNT];
    int i=0;
    // 开10000个线程
    int th_arg[THREAD_COUNT];
    for (i = 0; i < THREAD_COUNT; ++i)
    {
        th_arg[i] = i;
    }
    pthread_t thread[THREAD_COUNT];
    int ret_thrd[THREAD_COUNT];
    void *retval;
    for(i=0; i<THREAD_COUNT; i++){
        ret_thrd[i] = pthread_create(&thread[i], NULL, (void *)&print_message_function, &th_arg[i]);
        // 线程创建成功，返回0,失败返回失败号
        if (ret_thrd[i] != 0) {
            // printf("线程%d创建失败,errno:%d\n", i, errno);
            perror("create thread failed");
            return -1;
        }
    }

    for(i=0; i<THREAD_COUNT; i++){
        int tmp = pthread_join(thread[i], &retval);
        if (tmp != 0) {
            printf("cannot join with thread[%d]\n", i);
        } 
        // else {
        //     printf("ok!\n");
        //     printf("retval:%d\n", *(int *)retval);
        // }
    }

    pthread_mutex_destroy(&success_lock);
    pthread_mutex_destroy(&failed_lock);

    printf("success_count:%d\n", success_count);
    printf("failed_count:%d\n", failed_count);
    //
    // for(i=0;i<COUNT;i++)
    // {
    //     if((sockfd[i]=socket(AF_INET,SOCK_STREAM,0))==-1)//创建套接字
    //     {
    //         printf("第%d个套接字创建失败!\n", i);
    //         printf("errno=%d\n",errno);
    //         return -1;
    //     }
    // }

    // printf("create socker finished\n");
    printf("\n");
    return 0;
}

