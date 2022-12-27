// CMPUT 379 Fall 2022
// Assignment 3
// Author: Ke Li

// cite:
// learn the basic knowledge of socket in:
// https://www.youtube.com/playlist?list=PLPyaR5G9aNDvs6TtdpLcVO43_jvxp4emI
//
// how to get the host name:
// https://www.geeksforgeeks.org/c-program-display-hostname-ip-address/
//
// server inspired by:
// https://www.youtube.com/watch?v=fNerEo6Lstw

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
#include "clientHeader.h"
#include <netdb.h>
#include <sys/time.h>
#include <math.h>

// setting select()
fd_set current_socket, read_sockets, write_sockets;


int main(int argc, char *argv[])
{
    FILE *fp;

    int sockfd = 0, n = 0, portnum, j, bytes, Transnum = 0;
    char recvBuff[1024], buffer[256], sendBuff[1024];
    struct sockaddr_in serv_addr;
    struct timeval te;

    // check command line input
    if (argc != 3)
    {
        fprintf(stderr, "\n Usage: %s <port number> <id of server>\n", argv[0]);
        exit(1);
    }
    // socket setting
    memset(recvBuff, '0', sizeof(recvBuff));
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        error("[CLIENT] Error in opening Socket.\n");
    }
    memset(&serv_addr, '0', sizeof(serv_addr));
    portnum = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portnum);


    // initialize current set
    FD_ZERO(&current_socket);
    FD_SET(sockfd, &current_socket);

    // inet_pton
    // convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, argv[2], &serv_addr.sin_addr) <= 0)
    {
        error("[CLIENT] Error in inet_pton.\n");
    }

    // connect to server
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        error("[CLIENT] Failed in Connection.\n");
    }

    // send machinename.pid to the server
    int hostname, pid, namelen;
    char strpid[16];
    hostname = gethostname(buffer, 32);
    strcat(buffer, ".");
    pid = getpid();
    sprintf(strpid, "%d", pid);
    strcat(buffer, strpid);
    namelen = strlen(buffer);

    // fprintf to the file
    char filename[256];
    strcat(filename, buffer);
    strcat(filename, ".log");
    fp = fopen(filename, "w+");

    // print header msg
    fprintf(fp, "Using port %d\n", portnum);
    fprintf(fp, "Using server address %s\n", argv[2]);
    fprintf(fp, "Host %s\n", buffer);

    // set temp select fd_set
    read_sockets = current_socket;
    write_sockets = current_socket;
    // select write
    if (select(sockfd + 1, NULL, &write_sockets, NULL, NULL) < 0)
    {
        error("[SERVER] Error in select().\n");
    }
    // send machinename.pid
    send(sockfd, buffer, 256, 0);
    
    // write
    while (1)
    {
        // set temp select fd_set
        read_sockets = current_socket;
        write_sockets = current_socket;

        char input_temp[10];
        // printf("Plz enter your msg: ");
        j = scanf("%s", input_temp);

        // if control + d or end of the input,
        // stop read from stdin
        if (j == EOF)
        {
            break;
        }

        // check if the ouput can be added to the queue
        if (input_temp[0] == 'T')
        {
            char *strint = input_temp + 1;

            if (atoi(strint) > 0)
            {
                int num = atoi(strint);
                // how to get 2 digits after decimal:
                // https://stackoverflow.com/questions/35069863/how-to-divide-2-int-in-c
                // how to get epoch time:
                // https://www.folkstalk.com/tech/c-get-time-in-milliseconds-with-code-examples/
                gettimeofday(&te, NULL);
                long long millis = te.tv_sec * 1000LL + te.tv_usec / 1000;
                double Trantime = (double)millis / (double)1000;

                // send to server
                send(sockfd, strint, 256, 0);

                fprintf(fp, "%.2f: Send (T %d)\n", (double)Trantime, num);

                // append # of Trans
                Transnum++;

                // recv from server
                char strIndex[256];
                int index;
                recv(sockfd, strIndex, 256, 0);

                index = atoi(strIndex);

                gettimeofday(&te, NULL);
                millis = te.tv_sec * 1000LL + te.tv_usec / 1000;
                Trantime = (double)millis / (double)1000;
                fprintf(fp, "%.2f: Recv (D %d)\n", (double)Trantime, index);
            }
        }
        else if (input_temp[0] == 'S')
        {
            char *strint = input_temp + 1;

            if (atoi(strint) > 0)
            {
                int num = atoi(strint);
                fprintf(fp, "Sleep %d units\n",num);
                Sleep(num);
            }
        }
    }
    close(sockfd);
    fprintf(fp, "Sent %d transactions.\n", Transnum);

    return 0;
}

// inspired by:
// https://www.youtube.com/playlist?list=PLPyaR5G9aNDvs6TtdpLcVO43_jvxp4emI

void error(const char *msg)
{
    perror(msg);
    exit(1);
}