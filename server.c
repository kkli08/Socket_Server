// CMPUT 379 Fall 2022
// Assignment 3
// Author: Ke Li

// cite:
// learn the basic knowledge of socket in:
// https://www.youtube.com/playlist?list=PLPyaR5G9aNDvs6TtdpLcVO43_jvxp4emI
//
// server inspired by:
// https://www.youtube.com/watch?v=fNerEo6Lstw
//
// socket() inspired by:
// https://www.youtube.com/watch?v=Y6pFtgRdUts
//
// multithreads & testing
// https://www.youtube.com/watch?v=Pg_4Jz8ZIH4&list=PL9IEJIKnBJjH_zM5LnovnoaKlXML5qh17&index=5
//
// how to avoid deadlock:
// https://wiki.sei.cmu.edu/confluence/display/c/POS51-C.+Avoid+deadlock+with+POSIX+threads+by+locking+in+predefined+order
// https://cs61.seas.harvard.edu/site/2018/Synch5/
//
// select(timeout) inspired by:
// https://www.oreilly.com/library/view/hands-on-network-programming/9781789349863/8e8ea0c3-cf7f-46c0-bd6f-5c7aa6eaa366.xhtml

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
#include "serverHeader.h"
#include <signal.h>
#include <pthread.h>
#include <sys/time.h>
#include <math.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <net/if.h>


#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024
#define NAME_LEN 256

int client_count = 0;
int uid = 1; //  global id for each connection.
struct timeval te;
int IndexOfT = 1;

int cond_timeout = 0;
double server_start = 9999999999; // setting the start time
double cpu_time_use = 0;

FILE *fp;

char **SummaryArray;

Client *clients[MAX_CLIENTS];

// setting select()
fd_set current_socket, read_sockets, write_sockets;

// setting timeout
struct timeval timeout;

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t index_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t summary_mutex = PTHREAD_MUTEX_INITIALIZER;
// mutex for read and write
pthread_mutex_t mutex_readfd = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_writefd = PTHREAD_MUTEX_INITIALIZER;

// signal alarm:
// https://stackoverflow.com/questions/40519051/c-program-wont-terminate-after-30-seconds
static void ALARMhandler(int sig)
{
    printf("Time ran out!\nexiting program...\n");
    fprintf(fp, "\nSUMMARY\n");

    // printf("SummaryArray[0] = %s\n", SummaryArray[0]);

    for (int i = 0; i < client_count; i++)
    {
        if (SummaryArray[i])
        {
            fprintf(fp, "%s", SummaryArray[i]);
            free(SummaryArray[i]);
        }
    }
    double TPS = (IndexOfT - 1) / cpu_time_use;
    fprintf(fp, "%.1f transactions/sec  (%d/%.2f)\n", TPS, IndexOfT - 1, cpu_time_use);
    free(SummaryArray);
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
    // check command line input
    if (argc != 2)
    {
        fprintf(stderr, "\n Usage: %s <port number> \n", argv[0]);
        exit(1);
    }

    
    // declare variables
    int sockfd = 0, connfd = 0;
    struct sockaddr_in server_addr, client_addr;
    int portnum;
    pthread_t th;
    SummaryArray = malloc(MAX_CLIENTS * sizeof(*SummaryArray));

    // open fprintf file
    fp = fopen("server.log", "w+");

    // socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("[SERVER] Error in opening Socket.\n");

    memset(&server_addr, '0', sizeof(server_addr));

    // testing purpose
    // // get server ip address:
    // // https://www.sanfoundry.com/c-program-get-ip-address/
    // struct ifreq ifr;
    // char array[] = "eth0";
    // //Type of address to retrieve - IPv4 IP address
    // ifr.ifr_addr.sa_family = AF_INET;
    // //Copy the interface name in the ifreq structure
    // strncpy(ifr.ifr_name , array , IFNAMSIZ - 1);
    // ioctl(sockfd, SIOCGIFADDR, &ifr);
    // //display result
    // printf("IP Address is %s - %s\n" , array , inet_ntoa(( (struct sockaddr_in *)&ifr.ifr_addr )->sin_addr) );

    // socket settings
    portnum = atoi(argv[1]);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(portnum);

    // bind
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        error("[SERVER] Binding Failed.\n");

    // listen
    if (listen(sockfd, 10) < 0)
        error("[SERVER] Listening Failed.\n");

    fprintf(fp, "Using port %d\n", portnum);

    // initialize current set
    FD_ZERO(&current_socket);
    FD_SET(sockfd, &current_socket);

    // initialize timeout sec
    timeout.tv_sec = 30;
    timeout.tv_usec = 0;

    uid = sockfd;

    signal(SIGALRM, ALARMhandler);

    while (1)
    {
        // 30 sec timeout
        alarm(30);

        printf("Wait for a connection...\n");
        socklen_t clilen = sizeof(client_addr);
        connfd = accept(sockfd, (struct sockaddr *)&client_addr, &clilen);
        printf("Connection made!\n");

        if ((client_count + 1) >= MAX_CLIENTS)
        {
            printf("Max clients reached. Rejected.\n");
            close(connfd);
            continue;
        }

        // FD_SET add file descriptor to set
        FD_SET(connfd, &current_socket);

        // client setting
        Client *cli = (Client *)malloc(sizeof(Client));
        pthread_mutex_lock(&clients_mutex);
        cli->address = client_addr;
        cli->sockfd = connfd;
        cli->uid = uid++;
        cli->Tnums = 0;

        pthread_mutex_unlock(&clients_mutex);

        // add client to the queue
        enqueue(cli);

        // add client and fork thread
        pthread_create(&th, NULL, &client_handler, (void *)cli);
        // fprintf(stdout, "*************break point [3]*************\n");
    }

    return EXIT_SUCCESS;
}

// inspired by:
// https://www.youtube.com/playlist?list=PLPyaR5G9aNDvs6TtdpLcVO43_jvxp4emI

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

// thread handler function
void *client_handler(void *arg)
{
    char buffer[BUFFER_SIZE];
    int quit = 0, n;
    char machinename[NAME_LEN];

    Client *cli = (Client *)arg;

    // set temp select fd_set
    read_sockets = current_socket;
    write_sockets = current_socket;

    // select read fd
    int sockfd = uid + 1;
    // fprintf(stdout, "*************break point [0]*************\n");
    if (select(sockfd, &read_sockets, NULL, NULL, NULL) < 0)
    {
        error("[SERVER] Error in select().\n");
    }

    // machinename.pid
    // NOTE: in client, send the machinename.pid to the server first
    pthread_mutex_lock(&mutex_readfd);
    if (recv(cli->sockfd, machinename, NAME_LEN, 0) <= 0)
    {
        printf("[SERVER] Something bad happened when recieving data.\n");
        quit = 1;
    }
    else
    {
        strcpy(cli->name, machinename);
        printf("Successfully connect client(%s)!\n", cli->name);
    }
    pthread_mutex_unlock(&mutex_readfd);

    // main loop
    while (1)
    {
        if (quit)
        {
            break;
        }

        // set temp select fd_set
        read_sockets = current_socket;
        write_sockets = current_socket;

        // fprintf(stdout, "*************break point [1]*************\n");
        // select read fd
        if (select(sockfd, &read_sockets, NULL, NULL, NULL) < 0)
        {
            error("[SERVER] Error in select(). break point 1\n");
        }
        int recieve = recv(cli->sockfd, buffer, 256, 0);

        if (recieve > 0)
        {
            // append cli->Tnums
            cli->Tnums++;

            // each time send the T num
            int Tnum = atoi(buffer);

            // send Recv to client
            char strIndex[64];
            sprintf(strIndex, "%d", IndexOfT);

            // send to the client
            // pthread_mutex_lock(&mutex_writefd);
            send(cli->sockfd, strIndex, 256, 0);
            // pthread_mutex_unlock(&mutex_writefd);

            // print Tran # message
            // get the time
            gettimeofday(&te, NULL);
            long long millis = te.tv_sec * 1000LL + te.tv_usec / 1000;
            double Trantime = (double)millis / (double)1000;

            if (Trantime < server_start){
                server_start = Trantime;
            }
            snprintf(buffer, 14, "%f", Trantime);

            pthread_mutex_lock(&index_mutex);
            fprintf(fp, "%s: # %3d ( T %3d) from %s\n", buffer, IndexOfT, Tnum, cli->name);
            bzero(buffer, BUFFER_SIZE);

            // Tran
            Trans(Tnum);

            // print DONE message
            gettimeofday(&te, NULL);
            millis = te.tv_sec * 1000LL + te.tv_usec / 1000;
            Trantime = (double)millis / (double)1000;

            double update_end; 
            update_end = Trantime - server_start;
            if (cpu_time_use < update_end){
                cpu_time_use = update_end;
            }

            snprintf(buffer, 14, "%f", Trantime);
            

            fprintf(fp, "%s: # %3d ( DONE ) from %s\n", buffer, IndexOfT, cli->name);

            IndexOfT++;
            pthread_mutex_unlock(&index_mutex);
        }
        else if (recieve == 0)
        {
            quit = 1;
        }
        else
        {
            quit = 1;
        }

        bzero(buffer, BUFFER_SIZE);
    }

    // add summary to the summarylist
    // cite:
    // https://stackoverflow.com/questions/42795727/add-string-to-array-c-pointers
    char Tnumstr[64],
        summary[1024];

    sprintf(Tnumstr, "%d", cli->Tnums);
    bzero(summary, 1024);

    strcat(summary, "   ");
    strcat(summary, Tnumstr);
    strcat(summary, " transactions from ");
    strcat(summary, cli->name);
    strcat(summary, "\n");

    printf("[summary]%s\n", summary);
    pthread_mutex_lock(&summary_mutex);
    // SummaryArray[client_count] = summary;
    // SummaryArray = realloc(SummaryArray, MAX_CLIENTS * sizeof(*SummaryArray));
    // printf("->>>>hello im here\n");
    SummaryArray[client_count] = malloc(1024 * sizeof(char));
    strcpy(SummaryArray[client_count], summary);
    // printf("[assign to array]%s\n", SummaryArray[client_count]);
    // strcpy(cli->summary, summary);
    client_count++;
    pthread_mutex_unlock(&summary_mutex);

    // FD_CLR remove file descriptor from set
    FD_CLR(cli->sockfd, &current_socket);

    // delete client & thread
    printf("disconnected client(%s)...\n", cli->name);
    close(cli->sockfd);
    // dequeue(cli->uid);
    free(cli);

    // detach the thread and reclaimed it in the next interation
    pthread_detach(pthread_self());

    return NULL;
}

// inspired by:
// https://www.youtube.com/watch?v=fNerEo6Lstw

// enqueue the clients
void enqueue(Client *cli)
{
    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (!clients[i])
        {
            clients[i] = cli;
            break;
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

// inspired by:
// https://www.youtube.com/watch?v=fNerEo6Lstw

// dequeue the clients
void dequeue(int uid)
{
    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (clients[i])
        {
            if (clients[i]->uid == uid)
            {
                clients[i] = NULL;
                break;
            }
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}