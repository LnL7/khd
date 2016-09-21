#include "daemon.h"
#include "parse.h"

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

#define internal static
#define local_persist static

internal int KhdSockFD;
internal bool IsRunning;
internal int Port = 3021;
internal pthread_t Thread;

// NOTE(koekeishiya): Caller frees memory. */
char *ReadFromSocket(int SockFD)
{
    int Length = 256;
    char *Result = (char *) malloc(Length);

    Length = recv(SockFD, Result, Length, 0);
    if(Length > 0)
    {
        Result[Length] = '\0';
    }
    else
    {
        free(Result);
        Result = NULL;
    }

    return Result;
}

void WriteToSocket(const char *Message, int SockFD)
{
    send(SockFD, Message, strlen(Message), 0);
}

void CloseSocket(int SockFD)
{
    shutdown(SockFD, SHUT_RDWR);
    close(SockFD);
}

internal void *
HandleConnection(void *)
{
    while(IsRunning)
    {
        struct sockaddr_in ClientAddr;
        local_persist socklen_t SinSize = sizeof(struct sockaddr);

        int SockFD = accept(KhdSockFD, (struct sockaddr*)&ClientAddr, &SinSize);
        if(SockFD != -1)
        {
            char *Message = ReadFromSocket(SockFD);
            if(Message)
            {
                ParseKhd(Message);
                free(Message);
            }

            CloseSocket(SockFD);
        }
    }

    return NULL;
}

bool ConnectToDaemon(int *SockFD)
{
    struct sockaddr_in SrvAddr;
    struct hostent *Server;

    if((*SockFD = socket(PF_INET, SOCK_STREAM, 0)) == -1)
        return false;

    Server = gethostbyname("localhost");
    SrvAddr.sin_family = AF_INET;
    SrvAddr.sin_port = htons(Port);
    memcpy(&SrvAddr.sin_addr.s_addr, Server->h_addr, Server->h_length);
    memset(&SrvAddr.sin_zero, '\0', 8);

    return connect(*SockFD, (struct sockaddr*) &SrvAddr, sizeof(struct sockaddr)) != -1;
}

void TerminateDaemon()
{
    IsRunning = false;
    CloseSocket(KhdSockFD);
}

bool StartDaemon()
{
    struct sockaddr_in SrvAddr;
    int _True = 1;

    if((KhdSockFD = socket(PF_INET, SOCK_STREAM, 0)) == -1)
        return false;

    if(setsockopt(KhdSockFD, SOL_SOCKET, SO_REUSEADDR, &_True, sizeof(int)) == -1)
        printf("Could not set socket option 'SO_REUSEADDR'!\n");

    SrvAddr.sin_family = AF_INET;
    SrvAddr.sin_port = htons(Port);
    SrvAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    memset(&SrvAddr.sin_zero, '\0', 8);

    if(bind(KhdSockFD, (struct sockaddr*)&SrvAddr, sizeof(struct sockaddr)) == -1)
        return false;

    if(listen(KhdSockFD, 10) == -1)
        return false;

    IsRunning = true;
    pthread_create(&Thread, NULL, &HandleConnection, NULL);
    return true;
}
