#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdlib.h>
#include "data.h"
#include <math.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/epoll.h>
#define __USE_POSIX
//#define __USE_BSD
#include <signal.h>
#include <setjmp.h>
#include <database.h>
#define portA 6666
#define portB 7777
#define MESSAE_LENGTH 1000
#define REQUEST_LENGTH 100
#define ANUM 100
#define BNUM 1000000

#define STORE_HOUR 3
#define ERROR_ACTION(X)                                   \
    if (X == -1)                                          \
    {                                                     \
        printf("error %s:%d", strerror(errno), __LINE__); \
        exit_database();                                  \
        exit(1);                                          \
    }
pthread_mutex_t socknum_lock;
int gepfd[3];
static int signum;
time_t clock_after;
struct epoll_event b_revents[BNUM * 2];
//struct epoll_event b_wevents[BNUM];
typedef struct a
{
    int fd;
    struct sockaddr_in client;
    void *message;
    void *name;
    int pos;
    unsigned int wood_time;
} Info;
//不同板子的连接共享一份数据是不行的，在一个进程多个连接时必须想办法把数据和每个连接绑定
unsigned int curA, curB;
extern struct list head;
sigjmp_buf env;
char *ip_addr = "0.0.0.0";
int listenA, listenB;
void clean_sock(void)
{
    close(listenA);
    close(listenB);
}
void sigAlarmHandler(int signo)
{
    signum = signo;
    printf("%s,waiting for sensor infomation timeout!\n", strerror(errno));
    siglongjmp(env, 1);
}
Info *records[ANUM];
void *Aread(void *arg)
{
    struct epoll_event a_events[ANUM];
    struct epoll_event ev;
    char type[20] = {0};
    //char name[20]={0};//这传进节点导致了一个非常典型的bug
    char *name, *message_box;
    int epfd = (int)arg;
    int nfds;
    int n;
    int socket;
    Info *a_info;
    int i;
    while (1)
    {
        if (sigsetjmp(env, 1) != 0)
        {

            //puts("testtesttest");
            int i;
            for (i = 0; i < curA; i++)
            {
                //puts("testtesttest");
                a_info = records[i];
                epoll_ctl(epfd, EPOLL_CTL_DEL, a_info->fd, NULL);
                deleteNodeByName(a_info->name);
                close(a_info->fd);
                free(a_info);
            }
            curA = 0;
            continue;
        }
        nfds = epoll_wait(epfd, a_events, ANUM, -1);
        if (nfds < 0)
        {
            perror("epoll wait failed in Aread");
            printf("%d", errno);
            if (errno == 4)
            {
                continue;
            }
            exit_database();
            exit(1);
        }
        for (i = 0; i < nfds; i++)
        {
            if (a_events[i].events & EPOLLIN)
            {
                a_info = (Info *)(a_events[i].data.ptr);
                if (a_info == NULL)
                {
                    printf("error in A read:NULL pointer\n");
                    exit(1);
                }
                name = a_info->name;
                message_box = a_info->message;
                unsigned int len;
                n = recv(a_info->fd, &len, sizeof(int), MSG_WAITALL);

                if (n == 0 | n < 0)
                {
                    if (n < 0 && errno != EBADF)
                    {
                        alarm(0);
                        char *p = malloc(20);
                        printf("connection with %s:%d error:%s\n", inet_ntop(AF_INET, &a_info->client.sin_addr, p, 20), ntohs(a_info->client.sin_port), strerror(errno));

                        if (epoll_ctl(epfd, EPOLL_CTL_DEL, a_info->fd, NULL) == -1)
                        {
                            printf("A read epoll del failed %d:%s", __LINE__, strerror(errno));
                            free(p);
                            exit_database();
                            exit(1);
                        }
                        curA--;
                        deleteNodeByName(name);
                        close(a_info->fd);
                        free(p);
                        free(a_info);
                        continue;
                    }
                    else if (n < 0 && errno == EBADF)
                    {
                        perror("recv failed in A");
                        close(a_info->fd);
                        free(a_info);
                        exit_database();
                        exit(1);
                    }
                    else if (n == 0)
                    {
                        alarm(0);
                        char *p = malloc(20);
                        printf("board[%s:%d] has disconnected:%s\n", inet_ntop(AF_INET, &a_info->client.sin_addr, p, 20), ntohs(a_info->client.sin_port), strerror(errno));
                        free(p);
                        if (epoll_ctl(epfd, EPOLL_CTL_DEL, a_info->fd, NULL) == -1)
                        {
                            printf("A read epoll del failed %d:%s", __LINE__, strerror(errno));
                            exit_database();
                            exit(1);
                        }
                        curA--;
                        puts(name);
                        deleteNodeByName(name);
                        close(a_info->fd);
                        free(a_info);
                        continue;
                    }
                }
                len = ntohl(len);
                n = recv(a_info->fd, message_box, len, MSG_WAITALL);

                if (n == 0 | n < 0)
                {
                    if (n < 0 && errno != EBADF)
                    {
                        alarm(0);
                        char *p = malloc(20);
                        printf("connection with board %s:[%s:%d] error:%s\n", name, inet_ntop(AF_INET, &a_info->client.sin_addr, p, 20), ntohs(a_info->client.sin_port), strerror(errno));

                        if (epoll_ctl(epfd, EPOLL_CTL_DEL, a_info->fd, NULL) == -1)
                        {
                            printf("A read epoll del failed %d:%s", __LINE__, strerror(errno));
                            free(p);
                            exit_database();
                            exit(1);
                        }
                        curA--;
                        deleteNodeByName(name);
                        close(a_info->fd);
                        free(p);
                        free(a_info);
                    }
                    else if (n < 0 && errno == EBADF)
                    {
                        perror("recv failed in A");
                        close(a_info->fd);
                        free(a_info);
                        exit_database();
                        exit(1);
                    }
                    else if (n == 0)
                    {
                        alarm(0);
                        char *p = malloc(20);
                        printf("board %s:[%s:%d] has disconnected:%s\n", name, inet_ntop(AF_INET, &a_info->client.sin_addr, p, 20), ntohs(a_info->client.sin_port), strerror(errno));
                        free(p);
                        if (epoll_ctl(epfd, EPOLL_CTL_DEL, a_info->fd, NULL) == -1)
                        {
                            printf("A read epoll del failed %d:%s", __LINE__, strerror(errno));
                            exit_database();
                            exit(1);
                        }
                        curA--;
                        puts("normal disconnect!");
                        deleteNodeByName(name);
                        close(a_info->fd);
                        free(a_info);
                    }
                }
                else
                {
                    message_box[n] = 0;
                    strncpy(type, message_box, 10);
                    if (strncmp(type, "{\"sensor\":", 10) == 0)
                    {
                        unsigned int wood_time = a_info->wood_time;
                        if (wood_time == 0)
                        {
                            wood_time = a_info->wood_time = time(NULL);
                            printf("\n\n\njust for one time should it be\n\n\n");
                            save_board_data(message_box);
                        }
                        clock_after = time(NULL);
                        unsigned int sec = difftime(clock_after, wood_time);
                        if (sec >= 60 * 60 * STORE_HOUR)
                        {
                            printf("\n\n\nsecsecsec:%d\n\n\n", sec);
                            save_board_data(message_box);
                            a_info->wood_time = clock_after;
                        }
                        strncpy(type, message_box, 10);
                        sscanf(message_box, "{\"sensor\":[{\"clientName\":%[^,]", name);
                        addSensorInfomation(name, message_box);
                        alarm(50);
                    }

                    else if (strncmp(type, "{\"logout\":", 10) == 0)
                    {
                        alarm(0);
                        char *p = malloc(20);
                        printf("%s:%d logout:%s\n", inet_ntop(AF_INET, &a_info->client.sin_addr, p, 20), ntohs(a_info->client.sin_port), strerror(errno));
                        free(p);
                        epoll_ctl(epfd, EPOLL_CTL_DEL, a_info->fd, &ev);
                        deleteNodeByName(name);
                        close(a_info->fd);
                        free(a_info);
                    }
                }
            }
        }
    }
}
void *Bread(void *arg)
{
    struct epoll_event ev;
    int epfd = (int)arg;
    int nfds;
    int n;
    char request[REQUEST_LENGTH];
    char type[20] = {0};
    char board_name[50] = {0};
    char client_name[50] = {0};
    Info *b_info;
    int fd;
    int i;
    char *reply = "[\"error\":{\"body\":\"client not found!\"}]";
    while (1)
    {
        nfds = epoll_wait(epfd, b_revents, BNUM, -1);
        switch (nfds)
        {
        case -1:
            perror("epoll wait failed in Bread");
            printf("%d\n", errno);
            if (errno == 4)
            {
                continue;
            }
            exit_database();
            exit(1);
        case 0:
            continue;
        default:
            for (i = 0; i < nfds; i++)
            {
                if (b_revents[i].events & EPOLLIN)
                {
                    b_info = (Info *)b_revents[i].data.ptr;
                    fd = b_info->fd;
                    n = recv(fd, request, REQUEST_LENGTH, MSG_WAITALL);
                    //printf("[Bread]%s\n", request);
                    if (n < 0 && errno != EBADF)
                    {
                        char *p = malloc(20);
                        time_t now;
                        now = time(NULL);
                        printf("%s:socket error:connection with board%s [%s:%d]:%s\n", asctime(localtime(&now)), b_info->name, inet_ntop(AF_INET, &b_info->client.sin_addr, p, 20), ntohs(b_info->client.sin_port), strerror(errno));
                        free(p);
                        ERROR_ACTION(epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &ev));
                        close(fd);
                        free(b_info->message);
                        free(b_info);
                        curB--;
                    }
                    else if (n < 0 && errno == EBADF)
                    {
                        perror("recv failed in Bread");
                        free(b_info->message);
                        free(b_info);
                        exit_database();
                        exit(1);
                    }
                    else if (n == 0)
                    {
                        char *p = malloc(20);
                        time_t now;
                        now = time(NULL);
                        printf("%s:client %s [%s:%d] has disconnected :%s\n", asctime(localtime(&now)), client_name, inet_ntop(AF_INET, &b_info->client.sin_addr, p, 20), ntohs(b_info->client.sin_port), strerror(errno));
                        //printf("errno=%d;err:%s\n", errno, strerror(errno));
                        free(p);
                        ERROR_ACTION(epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &ev));
                        close(fd);
                        free(b_info->message);
                        free(b_info);
                        curB--;
                    }
                    else
                    {
                        strncpy(type, request, 10);
                        if (strncmp(type, "\"request\"", 9) == 0)
                        {
                            int m;
                            sscanf(request, " \" request \" : { \" type \" :%[^,] , \" clientName \" : %[^,] , \"boardNAme\" : %[^}] } ", type, client_name, board_name);
                            strcpy(b_info->name, client_name);
                            //printf("\nclient name:%s\n", client_name);
                            DataNode node = findNode(board_name);
                            if (node == NULL)
                            {
                                //printf("debug:290\n");
                                unsigned int len = htonl(strlen(reply));
                                send(fd, (void *)&len, sizeof(int), 0);
                                m = send(fd, (void *)reply, strlen(reply), 0);
                                if (m == -1 && errno == EBADF)
                                {
                                    printf("debug:294\n");
                                    printf("error %s in %d", strerror(errno), __LINE__);
                                    exit(1);
                                }
                                else if (m == -1 && errno != EAGAIN)
                                {
                                    curB--;
                                    printf("debug:301\n");
                                    if (epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL) == -1)
                                    {
                                        printf("error %s:%d", strerror(errno), __LINE__);
                                        exit(1);
                                    }
                                    close(b_info->fd);
                                    free(b_info->message);
                                    free(b_info);
                                }
                                // else if (m == -1)
                                // {
                                //     printf("debug:313\n");
                                //     strcpy(b_info->message, reply);
                                //     ev.events = EPOLLOUT | EPOLLET;
                                //     if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) == -1)
                                //     {
                                //         printf("epoll MOD failed %d:%s", __LINE__, strerror(errno));
                                //         exit(1);
                                //     }
                                // }
                                // else if (m < strlen(reply))
                                // {
                                //     printf("debug:324\n");
                                //     memmove(b_info->message, reply + m, strlen(reply) - m);
                                //     ev.events = EPOLLOUT | EPOLLET;
                                //     if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) == -1)
                                //     {
                                //         printf("epoll MOD failed %d:%s", __LINE__, strerror(errno));
                                //         exit(1);
                                //     }
                                // }
                            }
                            else if (strcmp(type, "\"sensor\"") == 0)
                            {
                                unsigned int len = htonl(strlen(node->sensor));
                                //printf("debug:len=%d\n\n", len);
                                m = send(fd, (void *)&len, sizeof(int), 0);
                                m = send(fd, node->sensor, strlen(node->sensor), 0);
                                if (m == -1 && errno == EBADF)
                                {
                                    printf("error %s in %d", strerror(errno), __LINE__);
                                    exit(1);
                                }
                                else if (m == -1 && errno != EAGAIN)
                                {
                                    curB--;
                                    puts("debug in 406:wired send error");
                                    ERROR_ACTION(epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL));
                                    close(b_info->fd);
                                    free(b_info->message);
                                    free(b_info);
                                }
                                // else if (m == -1)
                                // {
                                //     strcpy(b_info->message, node->sensor);
                                //     ev.events = EPOLLOUT | EPOLLET;
                                //     if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) == -1)
                                //     {
                                //         printf("epoll MOD failed %d:%s", __LINE__, strerror(errno));
                                //         exit(1);
                                //     }
                                // }
                                // else if (m < strlen(node->sensor))
                                // {
                                //     strcpy(b_info->message, node->sensor + m);
                                //     ev.events = EPOLLOUT | EPOLLET;
                                //     if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) == -1)
                                //     {
                                //         printf("epoll MOD failed %d:%s", __LINE__, strerror(errno));
                                //         exit(1);
                                //     }
                                // }
                            }
                        }
                    }
                }
                else if (b_revents[i].events & EPOLLHUP)
                {
                    b_info = (Info *)b_revents[i].data.ptr;
                    time_t now = time(NULL);
                    char *p = calloc(20, 1);
                    printf("%s:socket error:connection with board%s [%s:%d]:%s\n", asctime(localtime(&now)), b_info->name, inet_ntop(AF_INET, &b_info->client.sin_addr, p, 20), ntohs(b_info->client.sin_port), strerror(errno));
                }
                // else if (b_revents[i].events && EPOLLOUT)
                // {
                //     char *message;
                //     b_info = (Info *)b_revents[i].data.ptr;
                //     fd = b_info->fd;
                //     message = b_info->message;
                //     int n = send(fd, message, strlen(message), 0);
                //     if (n == -1)
                //     {
                //         if (errno == EBADF)
                //         {
                //             printf("error %s:%d", strerror(errno), __LINE__);
                //             exit(1);
                //         }
                //         else if (errno != EAGAIN)
                //         {
                //             ERROR_ACTION(epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL));
                //             close(fd);
                //             free(message);
                //             free(b_info);
                //         }
                //     }
                //     else if (n < strlen(message))
                //     {
                //         memmove(b_info->message, b_info->message + n, strlen(b_info->message) - n);
                //     }
                //     else
                //     {
                //         ERROR_ACTION(epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &ev));
                //     }
                // }
            }
        }
    }
}
void *AThread(void *arg)
{
    struct sockaddr_in server, client;
    struct epoll_event ev;
    pthread_t aread;
    int connfd;
    int epfd;
    epfd = epoll_create(ANUM);
    if (epfd == -1)
    {
        perror("epfd create failed");
        exit(1);
    }
    gepfd[0] = epfd;
    Info *a_info;
    listenA = socket(AF_INET, SOCK_STREAM, 0);
    if (listenA == -1)
    {
        perror("error create TCP socket");
        exit(1);
    }
    memset(&server, 0, sizeof(struct sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_port = htons(portA);
    socklen_t client_addr_len = sizeof(struct sockaddr);
    if (inet_aton(ip_addr, &server.sin_addr) == 0)
    {
        perror("address transferring error");
        exit(1);
    }
    if (bind(listenA, (struct sockaddr *)&server, sizeof(server)) == -1)
    {
        perror("error while trying to bind on portA");
        exit(1);
    }
    if (listen(listenA, ANUM) == -1)
    {
        printf("%d\n", listenA);
        perror("error while trying to listen to A");
        exit(1);
    }
    pthread_create(&aread, NULL, Aread, (void *)epfd);
    while (1)
    {
        connfd = accept(listenA, (struct sockaddr *)&client, &client_addr_len);
        if (connfd < 0)
        {
            perror("error accepting from board");
            exit(1);
        }
        a_info = malloc(sizeof(Info));
        //a_info->message=malloc(sizeof(MESSAE_LENGTH));
        a_info->message = calloc(MESSAE_LENGTH, 1);
        a_info->name = calloc(50, 1);
        strcpy(a_info->name, "in connection yet");
        a_info->client = client;
        a_info->fd = connfd;
        a_info->wood_time = 0;
        ev.data.ptr = a_info;
        ev.events = EPOLLIN | EPOLLET;
        records[curA++] = a_info;
        epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev);
    }
}
void *BThread(void *arg)
{
    Info *b_info;
    pthread_t bread, bwrite;
    int repfd, wepfd;
    int fdpro;
    repfd = epoll_create(BNUM);
    wepfd = epoll_create(BNUM);
    if (repfd == -1)
    {
        perror("repfd create failed");
        exit(1);
    }
    if (wepfd == -1)
    {
        perror("wepfd create failed");
        exit(1);
    }
    gepfd[1] = repfd;
    gepfd[2] = wepfd;
    struct sockaddr_in server, client;
    struct epoll_event ev;
    int connfd;
    listenB = socket(AF_INET, SOCK_STREAM, 0);
    if (listenB == -1)
    {
        perror("error create TCP socket");
        exit(1);
    }
    pthread_create(&bread, NULL, Bread, (void *)repfd);
    //pthread_create(&bwrite,NULL,Bwrite,(void *)wepfd);
    memset(&server, 0, sizeof(struct sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_port = htons(portB);
    socklen_t client_addr_len = sizeof(struct sockaddr);
    if (inet_aton(ip_addr, &server.sin_addr) == 0)
    {
        perror("address transferring error");
        exit(1);
    }
    if (bind(listenB, (struct sockaddr *)&server, sizeof(server)) == -1)
    {
        perror("error while trying to bind on portA\n");
        exit(1);
    }
    if (listen(listenB, BNUM) == -1)
    {
        printf("%d\n", listenB);
        perror("error while trying to listen to B\n");
        exit(1);
    }
    while (1)
    {
        connfd = accept(listenB, (struct sockaddr *)&client, &client_addr_len);
        //printf("%s","beta of sigint!\n");
        if (connfd < 0)
        {
            perror("error accepting from android:");
            exit(1);
        }
        // fdpro = fcntl(connfd, F_GETFL);
        // fcntl(connfd, F_SETFL, fdpro | O_NONBLOCK);
        ev.events = EPOLLIN | EPOLLET;
        b_info = malloc(sizeof(Info));
        ev.data.ptr = b_info;
        b_info->message = calloc(MESSAE_LENGTH, 1);
        b_info->name = calloc(50, 1);
        strcpy(b_info->name, " in connection yet");
        b_info->client = client;
        b_info->fd = connfd;
        epoll_ctl(repfd, EPOLL_CTL_ADD, connfd, &ev);
        curB++;
    }
}
pthread_mutex_t freelock;
int ack;
void *betaFunc(void *arg)
{
    pthread_mutex_init(&freelock, NULL);
    //这里有同步问题
    DataList p;
    while (1)
    {
        puts("beta restart!");
        for (p = head.next;;)
        {
            //ERROR_ACTION(pthread_mutex_lock(&freelock));
            if (p->body != NULL)
            {
                printf("%s", "[Beta]");
                puts(p->body->sensor);
                printf("\n------------------------------CURRENT CONNECTED CLIENT:%d------------------------------\n", curB);
                sleep(3);
            }
            p = p->next;
            /*其实在这里,p很可能已经被释放掉了！（无法解决? */
            //ERROR_ACTION(pthread_mutex_unlock(&freelock));
            /* if(ack==1)
         {
             ack=0;
             break;
         }*/
        }
    }
}
void sigPipeHandler(int signo)
{
    signum = signo;
    printf("[recv SIGPIPE!]\n");
}
int main(void)
{
    struct sigaction sigpipe;
    sigemptyset(&sigpipe.sa_mask);
    sigpipe.sa_flags = 0;
    sigpipe.sa_flags |= SA_RESTART;
    sigpipe.sa_handler = sigPipeHandler;
    if (sigaction(SIGPIPE, &sigpipe, NULL) == -1)
    {
        perror("sigaction error:");
        exit(1);
    }
    struct sigaction sigalarm;
    sigemptyset(&sigalarm.sa_mask);
    sigalarm.sa_flags = 0;
    sigalarm.sa_handler = sigAlarmHandler;
    if (sigaction(SIGALRM, &sigalarm, NULL) == -1)
    {
        perror("sigaction error:");
        exit(1);
    }
    listInit();
    if (pthread_mutex_init(&socknum_lock, NULL) != 0)
    {
        perror("error while initializing thread lock!");
        exit(1);
    }
    database_init();
    atexit(clean_sock);
    atexit(exit_database);
    atexit(routine_delete);
    pthread_t pA, pB;
    pthread_t pC;
    pthread_create(&pA, NULL, AThread, NULL);
    pthread_create(&pB, NULL, BThread, NULL);
    pthread_create(&pC, NULL, betaFunc, NULL);
    pthread_join(pA, NULL);
    pthread_join(pB, NULL);
    pthread_join(pC, NULL);
}
