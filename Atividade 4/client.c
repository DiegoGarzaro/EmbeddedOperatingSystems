/**
 * Cliente
 * 
 * TE355 - Sistemas Operacionais Embarcados
 * Trabalho4 - Chat com criptografia
 * 
 * Autor: Diego R. Garzaro
 * GRR20172364
 *  
 * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "aes.h"

static const unsigned char key[] = "abcdefg1234567";
int keysize = 15;

EVP_CIPHER_CTX *ctx_en;
EVP_CIPHER_CTX *ctx_de;
unsigned int salt[] = {12345, 54321};

/* Server port */
#define PORT 4242

/* Buffer length */
#define BUFFER_LENGTH 4096
#define NAME_LENGHT 20

/* Server address */
#define SERVER_ADDR "192.168.100.89"

char name[20];

void *receive_handler(void *args)
{
    char buffer_in[BUFFER_LENGTH];
    int *sockfd = (int *)args;
    char *msg_decrypted;

    while (1)
    {
        int slen = recv(*sockfd, buffer_in, BUFFER_LENGTH, 0);
        msg_decrypted = (char *)aes_decrypt(ctx_de, buffer_in, &slen);
        if (slen > 0)
        {
            printf("%s", msg_decrypted);
            printf("-> ");
            fflush(stdout);
        }
        else if (slen == 0)
        {
            break;
        }
        memset(buffer_in, 0x0, BUFFER_LENGTH);
    }
    return NULL;
}

/*
 * Main execution of the client program of our simple protocol
 */
int main(int argc, char *argv[])
{

    /* Server socket */
    struct sockaddr_in server;
    /* Client file descriptor for the local socket */
    int sockfd;

    int len = sizeof(server);
    int slen;

    /* Receive buffer */
    char buffer_in[BUFFER_LENGTH];
    /* Send buffer */
    char buffer_out[BUFFER_LENGTH];

    unsigned char *msg_encrypted;

    ctx_en =  EVP_CIPHER_CTX_new();
    ctx_de = EVP_CIPHER_CTX_new();
    if (aes_init((unsigned char*)key, keysize, (unsigned char *)&salt, ctx_en, ctx_de)) {
        printf("Couldn't initialize AES cipher\n");
        return -1;
    }

    int validName;
    do
    {
        printf("Digite seu nome: ");
        fgets(name, NAME_LENGHT, stdin);
        validName = 1;

        int i;
        for (i = 0; i < NAME_LENGHT; i++)
        {
            if (name[i] == '\n')
            {
                name[i] = '\0';
                break;
            }
        }

        if (strlen(name) > NAME_LENGHT || strlen(name) < 2)
        {
            printf("Nome invalido! Digite um nome com 2 ate 20 caracteres\n");
            validName = 0;
        }
    } while (!validName);

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
    server.sin_addr.s_addr = inet_addr(SERVER_ADDR);
    memset(server.sin_zero, 0x0, 8);

    /* Tries to connect to the server */
    if (connect(sockfd, (struct sockaddr *)&server, len) == -1)
    {
        perror("Can't connect to server");
        return EXIT_FAILURE;
    }

    // Send client's name to the server
    char send_name[NAME_LENGHT];
    strcpy(send_name, name);
    strcat(send_name, "\n");
    int name_len = strlen(send_name);
    msg_encrypted = aes_encrypt(ctx_en, (unsigned char *)send_name, &name_len);
    send(sockfd, msg_encrypted, strlen(msg_encrypted), 0);

    // Welcome message to the client
    printf("Bem-vindo a sala de chat, %s!\n", name);

    pthread_t receive_thread;
    if (pthread_create(&receive_thread, NULL, &receive_handler, &sockfd) != 0)
    {
        printf("It was not possible to create the thread\n");
        return EXIT_FAILURE;
    }

    /*
     * Commuicate with the server until the exit message come
     */
    int isClose = 0;
    do
    {
        char buffer_out[BUFFER_LENGTH - NAME_LENGHT - 10];

        /* Zeroing the buffers */
        memset(buffer_out, 0x0, sizeof(buffer_out));

        printf("-> ");
        fflush(stdout);

        int isValidtoSend = 0;
        do{
            fgets(buffer_out, sizeof(buffer_out), stdin);
            if(strlen(buffer_out) > 1){
                isValidtoSend = 1;
            }
            else{
                printf("-> ");
                fflush(stdout);
            }
        } while(!isValidtoSend);

        int i;
        for (i = 0; i < sizeof(buffer_out); i++)
        {
            if (buffer_out[i] == '\n')
            {
                buffer_out[i] = '\0';
                break;
            }
        }

        if (strcmp(buffer_out, "exit") == 0)
        {
            isClose = 1;
        }
        else
        {
            char message[BUFFER_LENGTH];
            int msg_len = sizeof(message);
            sprintf(message, "%s: %s\n", name, buffer_out);
            msg_encrypted = aes_encrypt(ctx_en, (unsigned char *)message, &msg_len);
            send(sockfd, msg_encrypted, strlen(msg_encrypted), 0);
        }

        memset(buffer_out, 0x0, sizeof(buffer_out));

    } while (isClose == 0);

    /* Close the connection whith the server */
    close(sockfd);

    fprintf(stdout, "\nConnection closed\n\n");

    return EXIT_SUCCESS;
}