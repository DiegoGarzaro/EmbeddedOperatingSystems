/**
 * Client
 * 
 * Sistemas Operacionais Embarcados - Trabalho 3
 * 
 * Author: Diego R. Garzaro
 * GRR20172364
 * 
 * */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

/* Defines the server port */
#define PORT 8081

/* Server address */
#define SERVER_ADDR "127.0.0.1"

int valid(char *buffer);

int main(int argc, char *argv[])
{

    /* Server socket */
    struct sockaddr_in server;
    /* Client file descriptor for the local socket */
    int sockfd;

    int len = sizeof(server);
    int slen;

    /* Receive buffer */
    char buffer_in[PATH_MAX];
    /* Send buffer */
    char buffer_out[PATH_MAX];

    fprintf(stdout, "Starting Client ...\n");

    /*
     * Creates a socket for the client
     */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Error on client socket creation:");
        return EXIT_FAILURE;
    }
    fprintf(stdout, "Client socket created with fd: %d\n", sockfd);

    /* Defines the connection properties */
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);

    struct hostent *host;
    host = gethostbyname(SERVER_ADDR);
    if (!host)
    {
        fprintf(stderr, "Client error: unknown host\n");
        return -4;
    }
    memcpy(&server.sin_addr, host->h_addr_list[0], host->h_length);
    if (connect(sockfd, (struct sockaddr *)&server, sizeof(server)))
    {
        fprintf(stderr, "Client error: cannot connect to host\n");
        return -5;
    }

    /* Receives the test message from the server */
    if ((slen = recv(sockfd, buffer_in, PATH_MAX, 0)) > 0)
    {
        buffer_in[slen + 1] = '\0';
        fprintf(stdout, "Server Connection: %s\n", buffer_in);
    }

    /*
     * Commuicate with the server until the close message come
     */
    do
    {

        /* Cleaning the buffers */
        memset(buffer_in, 0x0, PATH_MAX);
        memset(buffer_out, 0x0, PATH_MAX);

        int i;
        do
        {
            fprintf(stdout, "\nClient: ");
            fgets(buffer_in, PATH_MAX, stdin);
            for (i = 0; i < strlen(buffer_in); i++)
            {
                if (buffer_in[i] == '\n')
                {
                    buffer_in[i] = '\0';
                    i = strlen(buffer_in) + 1;
                }
            }
            if(!valid(buffer_in)){
                printf("Invalid Command");
            }
        } while (!valid(buffer_in));

        /* Sends the read message to the server through the socket */
        send(sockfd, buffer_in, strlen(buffer_in), 0);

        if(strcmp(buffer_in, "close") == 0){
            break;
        }

        /* Receives an answer from the server */
        int message_len;

        int receaved = 0;
        printf("\nServer: ");
        do
        {
            memset(buffer_out, 0x0, PATH_MAX);
            if ((message_len = recv(sockfd, buffer_out, PATH_MAX, 0)) > 0)
            {
                //buffer[message_len - 1] = '\0';
                for (i = 0; i < strlen(buffer_out); i++)
                {
                    if (buffer_out[i] == '$' && buffer_out[i + 1] == '&' && buffer_out[i + 2] == '$')
                    {
                        receaved = 1;
                        buffer_out[i] = '\0';
                    }
                }
                printf("%s", buffer_out);
            }
        } while (receaved == 0);

    } while (1);

    /* Close the connection whith the server */
    close(sockfd);

    fprintf(stdout, "\nConnection closed\n\n");

    return EXIT_SUCCESS;
}

int valid(char *buffer){
    int i, j;
    int pos = 0;
    char command[100];
    memset(command, 0, 100);
    for (i = 0, j = 0; i < strlen(buffer); i++, j++)
    {
        if ((buffer[i] == ' ' || buffer[i] == '\0') && pos == 0)
        {
            j = 0;
            pos++;
        }
        else if(pos == 0)
        {
            command[j] = buffer[i];
        }
    }
    if(strcmp(command, "ls") == 0 || strcmp(command, "cd") == 0 || strcmp(command, "close") == 0 || strcmp(command, "rm") == 0 || strcmp(command, "mkdir") == 0 || strcmp(command, "rename") == 0){
        return 1;
    }
    return 0;
}