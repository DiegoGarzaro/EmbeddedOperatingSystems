/**
 * Server
 * 
 * Sistemas Operacionais Embarcados - Trabalho 3
 * 
 * Author: Diego R. Garzaro
 * GRR20172364
 * 
 * */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <linux/in.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>

/* Server Port */
#define PORT 8081

char current_path[PATH_MAX];

char *protocol(char *input, char *path);

typedef struct Connection_params
{
    int sock;
    struct sockaddr address;
    int addr_len;
} Connection;

void *myThread(void *ptr)
{
    FILE *fp;
    char buffer[PATH_MAX];
    int len;
    Connection *conn;
    long addr = 0;
    char answer[1000];
    int sair = 0;

    if (!ptr)
    {
        pthread_exit(0);
    }
    conn = (Connection *)ptr;
    /* Copies into buffer the message will confirm the connection */
    strcpy(buffer, "OK!\n\0");

    /* Sends the test message to the client */
    if (send(conn->sock, buffer, strlen(buffer), 0))
    {
        fprintf(stdout, "Client connected.\n\n");
    }
    /* Communicates with the client until close message come */
    do
    {

        /* Cleaning buffers */
        memset(buffer, 0x0, PATH_MAX);
        memset(answer, 0x0, 1000);

        /* Receives client message */
        int message_len;
        if ((message_len = recv(conn->sock, buffer, PATH_MAX, 0)) > 0)
        {
            printf("Client: %s", buffer);
        }
        if(strcmp(buffer, "close") == 0){
            printf("\n\nConnection closed\n\n");
            exit(1);
        }

        /* Initialize the protocol to handle with the receaved message */
        char *protocolresponse = protocol(buffer, current_path);
        if (strcmp(protocolresponse, "Invalid Command") != 0)
        {
            fp = popen(protocolresponse, "r");
            if (fp == NULL)
            {
                printf("Failed to run command\n");
                exit(1);
            }
            /* Run the command on the server */
            printf("Running command\n");
            /* Read the output of the server terminal and send to client */
            printf("\nResponse: \n");
            while (fgets(answer, sizeof(answer), fp) != NULL)
            {
                if(buffer[0] != 'c' && buffer[1] != 'd'){
                    printf("%s", answer);
                    write(conn->sock, answer, strlen(answer));
                }
            }
            if(buffer[0] != 'c' && buffer[1] != 'd'){
                write(conn->sock, "$&$", 3);
                //printf("\nResponse: %s\n", answer);
            }
        }
        else
        {
            /* Error message caused from an invalid command */
            printf("\nResponse: Invalid Command\n");
            write(conn->sock, "Invalid Command$&$", 18);
        }

        /* Update the current path when cd command is called */
        if (buffer[0] == 'c' && buffer[1] == 'd' && strlen(buffer) > 2)
        {
            int i, count, j;
            int start_args = 3;
            char args[PATH_MAX];

            for (i = 0, count = 0; i < strlen(buffer); i++)
            {
                if (buffer[i] == '/')
                {
                    count++;
                }
            }

            for (i = 0, j = start_args; i < strlen(buffer); i++, j++)
            {
                args[i] = buffer[j];
            }

            if (strcmp(buffer, "cd ..") == 0)
            {
                for (i = strlen(current_path); i > 0; i--)
                {
                    if (current_path[i] != '/')
                    {
                        current_path[i] = 0;
                    }
                    else
                    {
                        current_path[i] = '\0';
                        i = 0;
                    }
                }
            }
            else if (buffer[3] == '/')
            {
                strcpy(current_path, args);
            }
            else
            {
                strcat(current_path, "/");
                strcat(current_path, args);
                strcat(current_path, "\0");
            }
            strcpy(answer, current_path);
            strcat(answer, "$&$");
            printf("%s", answer);
            write(conn->sock, answer, strlen(answer));
        }

    } while (1);

    /* close socket and clean up */
    close(conn->sock);
    pclose(fp);
    free(fp);
    free(conn);
    pthread_exit(0);
}

int main(int argc, char **argv)
{
    int sock = -1;
    struct sockaddr_in address;
    Connection *connection;
    pthread_t thread;

    if (getcwd(current_path, sizeof(current_path)) != NULL)
    {
        printf("Current working dir: %s\n", current_path);
    }
    else
    {
        perror("getcwd() error");
        return 1;
    }

    /* create socket */
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock <= 0)
    {
        fprintf(stderr, "Server Error: cannot create socket\n");
        return -3;
    }

    /* bind socket to port */
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    address.sin_addr.s_addr = INADDR_ANY;
    if (bind(sock, (struct sockaddr *)&address, sizeof(struct sockaddr_in)) < 0)
    {
        fprintf(stderr, "Server Error: cannot bind socket to port %d\n", PORT);
        return -4;
    }

    /* listen on port */
    if (listen(sock, 5) < 0)
    {
        fprintf(stderr, "Server Error: cannot listen on port\n");
        return -5;
    }

    printf("Server Initialized\n");

    while (1)
    {
        /* accept incoming connections */
        connection = (Connection *)malloc(sizeof(Connection));
        connection->sock = accept(sock, &connection->address, &connection->addr_len);
        if (connection->sock <= 0)
        {
            free(connection);
        }
        else
        {
            /* start a new thread but do not wait for it */
            pthread_create(&thread, 0, myThread, (void *)connection);
            pthread_detach(thread);
        }
    }

    free(connection);
    return 0;
}

char *protocol(char *input, char *path)
{
    char command[10];
    char args[1000];
    int number = PATH_MAX;
    char *response = (char *)calloc(number, sizeof(char));

    memset(command, 0, 10);
    memset(args, 0, 1000);

    int i, j;
    int pos = 0;
    for (i = 0, j = 0; i < strlen(input); i++, j++)
    {
        if ((input[i] == ' ' || input[i] == '\0') && pos == 0)
        {
            j = 0;
            pos++;
        }
        else if (pos == 0)
        {
            command[j] = input[i];
        }
        if (pos >= 1)
        {
            args[j] = input[i];
        }
    }

    if (strcmp(command, "ls") == 0 || strcmp(command, "cd") == 0 || strcmp(command, "rm") == 0 || strcmp(command, "mkdir") == 0 || strcmp(command, "rename") == 0)
    {
        strcat(response, "cd ");
        strcat(response, path);
        strcat(response, " && ");
        if (strcmp(command, "cd") != 0)
        {
            if (strcmp(command, "rename") == 0)
            {
                strcat(response, "mv -v");
            }
            else
            {
                strcat(response, command);
                strcat(response, " -v");
            }
            strcat(response, args);
        }
        else
        {
            strcat(response, input);
        }
        printf("\nProtocol Command: %s\n", response);
        return response;
    }
    else
    {
        return "Invalid Command";
    }
}