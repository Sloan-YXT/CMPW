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
#include <signal.h>
#include <math.h>
/*
//Just for beta
#include <error.h>
#include <errno.h>
*/
#define portA 6666
#define portB 7777
#define MESSAE_LENGTH 1000
#define REQUEST_LENGTH 200
#define ANUM 10
#define BNUM 10
#define listenNum 10
unsigned int sockets[listenNum],socknum;
pthread_mutex_t socknum_lock;
//waiting for notify:problems when sockets quit
static void sigIntHandle(int signo)
{
    int i;
    printf("SIGINT RECVED:",signo);
    printf("server is exiting now......\n");
    printf("%d sockets are going to be shutdown!\n",socknum);
      for(i=0;i<listenNum;i++)
    {
        if(sockets[i]!=-1)
        close(sockets[i]);
    }
    pthread_mutex_destroy(&socknum_lock);
    exit(0);
}
int findFree(void)
{
    int i;
    for(i=0;i<listenNum;i++)
    {
        if(sockets[i]==-1)
        {
            return i;
        }
    }
    return -1;
}
extern struct list head;
 char *ip_addr = "0.0.0.0";
struct arg
{
    int connfd;
    pthread_t *p;
    struct sockaddr_in client;
    int pos;
};
void *pData(void *args)
{
    
    //pthread_detach(pthread_self());
    struct arg *info=(struct arg*)args;
    char *message_box=malloc(MESSAE_LENGTH);
    int n;
    char *handle=calloc(10,1);
    char *boardName=calloc(50,1);
    while(1)
    {
        n = recv(info->connfd,(void*)message_box,MESSAE_LENGTH,0);
        //puts(message_box);
        printf("[PData]%d bytes recved from %s!\n",n,inet_ntoa(info->client.sin_addr));
        if(n==-1)
        {
            sockets[info->pos]=-1;
             if(pthread_mutex_lock(&socknum_lock)!=0)
        {
            perror("lock failed in DataThread(exiting):");
            exit(1);
        }
            socknum--;
        if(pthread_mutex_unlock(&socknum_lock)!=0)
        {
            perror("unlock failed in DataThread(exiting):");
            exit(1);
        }
            perror("error reading sensor message!(the connection from board maybe shutdown:");
             deleteNodeByName(boardName);
            free(info);
            free(handle);
            free(message_box);
            if(close(info->connfd)==-1)
            {
                perror("error while closing data fd!\n");
            }
            return NULL;
        }
        if(n==0)
        {
            sockets[info->pos]=-1;
             if(pthread_mutex_lock(&socknum_lock)!=0)
        {
            perror("lock failed in DataThread(exiting):");
            exit(1);
        }
            socknum--;
        if(pthread_mutex_unlock(&socknum_lock)!=0)
        {
            perror("unlock failed in DataThread(exiting):");
            exit(1);
        }
           printf("%s is quiting now......\n",boardName);
            //perror("error reading sensor message!(the connection from board maybe shutdown\n");
            deleteNodeByName(boardName);
            free(info);
            free(handle);
            free(message_box);
            if(close(info->connfd)==-1)
            {
                perror("error while closing data fd!\n");
            }
            return NULL;
        }
        strncpy(handle,message_box,10);
        if(strncmp(handle,"{\"sensor\":",10)==0)
        {
            sscanf(message_box,"{\"sensor\":[{\"clientName\":%[^,]",boardName);
            puts(boardName);
            addSensorInfomation(boardName,message_box);
        }
        else if(strncmp(handle,"{\"logout\":",10)==0)
        {
            free(info);
            free(handle);
            free(message_box);
            if(close(info->connfd)==-1)
            {
                perror("error while closing data fd:");
            }
            pthread_exit(NULL);
        }  
    }
    
}
void *pRequest(void *args)
{
    //pthread_detach(pthread_self());
    struct arg * info;
    info = (struct arg*)args;
    char *reply = "[\"error\":{\"body\":\"client not found!\"}]";
    char *request = calloc(REQUEST_LENGTH,1);
    char *handle = calloc(10,1);
    char *type = calloc(20,1);
    char *board_name = calloc(50,1);
    char *client_name = calloc(50,1);
    int n;
    while(1)
    {
        n=recv(info->connfd,(void*)request,REQUEST_LENGTH,0);
        printf("[PRequest]%d bytes recved from %s!\n",n,inet_ntoa(info->client.sin_addr));
         if(n==-1)
        {
            sockets[info->pos]=-1;
             if(pthread_mutex_lock(&socknum_lock)!=0)
        {
            perror("lock failed in RequestThread(exiting):");
            exit(1);
        }
            socknum--;
        if(pthread_mutex_unlock(&socknum_lock)!=0)
        {
            perror("unlock failed in RequestThread(exiting):");
            exit(1);
        }
            perror("error while reading from android client!the connection maybe shutdown from the client:");
            free(request);
            free(handle);
            free(type);
            free(board_name);
            free(client_name);
            if(close(info->connfd)==-1)
            {
                perror("error while closing request fd!\n");
                exit(1);
            }
            free(info);
            return NULL;
        }
        if(n==0)
        {
            sockets[info->pos]=-1;
           if(pthread_mutex_lock(&socknum_lock)!=0)
        {
            perror("lock failed in RequestThread(exiting):");
            exit(1);
        }
            socknum--;
        if(pthread_mutex_unlock(&socknum_lock)!=0)
        {
            perror("unlock failed in RequestThread(exiting):");
            exit(1);
        }
            //perror("error while reading from android client!the connection maybe shutdown from the client\n");
            free(request);
            free(handle);
            free(type);
            free(board_name);
            free(client_name);
            if(close(info->connfd)==-1)
            {
                perror("error while closing request fd!\n");
                exit(1);
            }
            free(info);
            return NULL;
        }
        strncpy(handle,request,10);
        puts(request);
        if(strncmp(handle,"\"request:\"",10)==0);
    {
      sscanf(request," \" request \" : { \" type \" :%[^,] , \" clientName \" : %[^,] , \"boardNAme\" : %[^}] } ",type,client_name,board_name);
      puts(type);
      puts(client_name);
      puts(board_name);
      DataNode node =    findNode(board_name);
      if(node==NULL)
      {
          send(info->connfd,(void*)reply,strlen(reply),0);
      }
      else if(strcmp(type,"\"sensor\"")==0)
      {
          //puts("beta:in sending data\n");
          //puts(node->sensor);
          send(info->connfd,node->sensor,strlen(node->sensor),0);
      }
    }
    }
}
void* AThread(void *arg)
{
    struct arg * info;
    int pos;
    struct sockaddr_in server,client;
    int listenfd,connfd;
    listenfd=socket(AF_INET,SOCK_STREAM,0);
    if(listenfd==-1)
    {
        perror("error create TCP socket!");
        exit(1);
    }
    if(pthread_mutex_lock(&socknum_lock)!=0)
    {
        perror("lock failed in thread A(listenfd):");
        exit(1);
    };
    sockets[socknum++]=listenfd;
    if(pthread_mutex_unlock(&socknum_lock)!=0)
    {
        perror("unlock failed in thread A(listenfd):");
        exit(1);
    };
    memset(&server,0,sizeof(struct sockaddr_in));
    server.sin_family=AF_INET;
    server.sin_port=htons(portA);
    socklen_t client_addr_len = sizeof(struct sockaddr);
    if(inet_aton(ip_addr,&server.sin_addr)==0)
    {
        perror("address transferring error!\n");
        exit(1);
    }
    //server.sin_addr.s_addr=htonl(INADDR_ANY);
    if(bind(listenfd,(struct sockaddr*)&server,sizeof(server))==-1)
    {
        perror("error while trying to bind on portA!\n");
        exit(1);
    }
        if(listen(listenfd,ANUM)==-1)
    {
        printf("%d\n",listenfd);
        perror("error while trying to listen to A!\n");
        exit(1);
    }
    while(1)
    {
        connfd = accept(listenfd,(struct sockaddr*)&client,&client_addr_len);
        if(connfd<0)
        {
            perror("error accepting from board!\n");
            exit(1);
        }
        if(pthread_mutex_lock(&socknum_lock)!=0)
        {
            perror("lock failed in ThreadA(connfd):");
            exit(1);
        }
        socknum++;
        pos = findFree();
        if(pthread_mutex_unlock(&socknum_lock)!=0)
        {
            perror("unlock failed in ThreadA(connfd):");
            exit(1);
        }
        if(pos==-1)
        {
            printf("[AThread]boards accepting reaches max num!abort connectiong from %s......",inet_ntoa(client.sin_addr));
            continue;
        }
        info = malloc(sizeof(struct arg));
        info->client = client;
        info->connfd=connfd;
        pthread_t * p = malloc(sizeof(pthread_t));
        info->p=p;
        info->pos=pos;
        pthread_create(p,NULL,pData,info);
    }
}
void *BThread(void* arg)
{
    int pos;
    struct arg * info;
    struct sockaddr_in server,client;
    int listenfd,connfd;
    listenfd=socket(AF_INET,SOCK_STREAM,0);
    if(listenfd==-1)
    {
        perror("error create TCP socket:");
        exit(1);
    }
    if(pthread_mutex_lock(&socknum_lock)!=0)
    {
        perror("lock failed in ThreadB(listenfd):");
        exit(1);
    }
    sockets[socknum++]=listenfd;
    if(pthread_mutex_unlock(&socknum_lock)!=0)
    {
        perror("unlock failed in ThreadB(listenfd):");
        exit(1);
    }
    memset(&server,0,sizeof(struct sockaddr_in));
    server.sin_family=AF_INET;
    server.sin_port=htons(portB);
    socklen_t client_addr_len = sizeof(struct sockaddr);
    if(inet_aton(ip_addr,&server.sin_addr)==0)
    {
        perror("address transferring error:");
        exit(1);
    }
    //server.sin_addr.s_addr=htonl(INADDR_ANY);
    if(bind(listenfd,(struct sockaddr*)&server,sizeof(server))==-1)
    {
        perror("error while trying to bind on portB:");
        exit(1);
    }
    if(listen(listenfd,BNUM)==-1)
    {
        printf("%d %d\n",server.sin_port,server.sin_addr.s_addr&0xff);
        perror("error while trying to listen to B:");
        exit(1);
    }
    while(1)
    {
        connfd = accept(listenfd,(struct sockaddr*)&client,&client_addr_len);
        printf("%s","beta of sigint!\n");
        if(connfd<0)
        {
            perror("error accepting from android:");
            exit(1);
        }
        if(pthread_mutex_lock(&socknum_lock)!=0)
        {
            perror("lock failed in ThreadB(connfd):");
            exit(1);
        }
        pos = findFree();
        socknum++;
        if(pthread_mutex_unlock(&socknum_lock)!=0)
        {
            perror("unlock failed in ThreadB(connfd):");
            exit(1);
        }
        if(pos==-1)
        {
            printf("[BThread]clients accepting reaches max num!abort connectiong from %s......",inet_ntoa(client.sin_addr));
            continue;
        }
        info = malloc(sizeof(struct arg));
        info->client = client;
        info->connfd=connfd;
        pthread_t * p = malloc(sizeof(pthread_t));
        info->p=p;
        info->pos=pos;
        pthread_create(p,NULL,pRequest,info);
    }
}
void* betaFunc(void *arg)
{
    DataList p;
    for(p=head.next;;p=p->next)
    {
        if(p->body!=NULL)
        {
            printf("%s","[Beta]");
            puts(p->body->sensor);
            sleep(3);
        }
    }
}
int main(void)
{
    //memset(sockets,-1,listenNum);//doesn't work:returns INT_MAX
    int i;
    for(i=0;i<listenNum;i++)
    {
        sockets[i]=-1;
    }
    struct sigaction sigint;
    sigemptyset(&sigint.sa_mask);
    sigint.sa_flags=0;
    sigint.sa_handler = sigIntHandle;
    siginterrupt(SIGINT,1);
    if(sigaction(SIGINT,&sigint,NULL)==-1)
    {
        perror("sigaction error:");
        exit(1);
    }
    listInit();
    if(pthread_mutex_init(&socknum_lock,NULL)!=0)
    {
        perror("error while initializing thread lock!");
        exit(1);
    }
    pthread_t pA,pB;
    pthread_t pC;
    pthread_create(&pA,NULL,AThread,NULL);
    pthread_create(&pB,NULL,BThread,NULL);
    pthread_create(&pC,NULL,betaFunc,NULL);
    pthread_join(pA,NULL);
    pthread_join(pB,NULL);
    pthread_join(pC,NULL);
}