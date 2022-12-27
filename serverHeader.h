#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024
#define NAME_LEN 256

// client structure
typedef struct
{
    /* data */
    struct sockaddr_in address;
    int sockfd;
    int uid; // avoid deadlock & predefined order
    int Tnums;
    char name[NAME_LEN];
    // char summary[BUFFER_SIZE];
} Client;

void Trans(int n);
void Sleep(int n);
void error(const char *msg);
void *client_handler(void *arg);
void enqueue(Client *cli);
void dequeue(int uid);