/**
 * Servidor
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
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
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

/* Max number of clients */
#define MAX_CLIENTS 5

/* Client Structure */
typedef struct
{
	struct sockaddr_in address;
	int sockfd;
	int uid;
	char name[20];
} client_t;

client_t *clients[MAX_CLIENTS];

int uid = 0;

// Functions Prototype
void add_client(client_t *c, int pos);
void send_message_client(char *buffer, client_t *c);

// Thread function
void *client_handler(void *args)
{
	// code
	char buffer[BUFFER_LENGTH];
	char name[20];
	int buffer_len = 0;

	client_t *cliente = (client_t *)args;
	char *msg_decrypted;
	unsigned char *msg_encrypted;

	memset(buffer, 0x0, BUFFER_LENGTH);

	// Receive the name of the new client connected
	int receive = recv(cliente->sockfd, name, BUFFER_LENGTH, 0);
	msg_decrypted = (char *)aes_decrypt(ctx_de, name, &receive); 
	if (receive <= 0 || strlen(name) < 2 || strlen(name) >= 19)
	{
		printf("Nome de usuario invalido\n");
		strcpy(cliente->name, "Desconhecido");
		char error_message[] = "Nome de usuario invalido, entre com um nome compativel: ";
		int len = strlen(error_message);
		msg_encrypted = aes_encrypt(ctx_en, (unsigned char *)error_message, &len);
		write(cliente->sockfd, error_message, len);
	}
	else
	{
		strcpy(cliente->name, msg_decrypted);
		int i;
		for(i = 0; i < 20; i++){
			if(cliente->name[i] == '\n'){
				cliente->name[i] = '\0';
			}
		}
		sprintf(buffer, "%s entrou no chat\n", cliente->name);
		printf("%s", buffer);
		int len = BUFFER_LENGTH;
		msg_encrypted = aes_encrypt(ctx_en, (unsigned char *)buffer, &len);
		send_message_client(msg_encrypted, cliente);
	}

	memset(buffer, 0x0, BUFFER_LENGTH);

	// Chat Loop
	do
	{
		buffer_len = recv(cliente->sockfd, buffer, BUFFER_LENGTH, 0);
		msg_decrypted = (char *)aes_decrypt(ctx_de, buffer, &buffer_len); 
		memset(buffer, 0x0, BUFFER_LENGTH);
		strcpy(buffer, msg_decrypted);
		if (buffer_len > 0)
		{
			if (strlen(buffer) > 0)
			{
				int len = sizeof(buffer);
				msg_encrypted = aes_encrypt(ctx_en, (unsigned char *)buffer, &len);
				send_message_client(msg_encrypted, cliente);
				int i;
				for (i = 0; i < strlen(buffer); i++)
				{
					if (buffer[i] == '\n')
					{
						buffer[i] = '\0';
						break;
					}
				}
				printf("%s\n", buffer);
			}
		}
		else if (buffer_len == 0 || strcmp(buffer, "exit") == 0)
		{
			sprintf(buffer, "%s saiu desta sala de chat.\n", cliente->name);
			printf("%s", buffer);
			int len = sizeof(buffer);
			msg_encrypted = aes_encrypt(ctx_en, (unsigned char *)buffer, &len);
			send_message_client(msg_encrypted, cliente);
		}
		memset(buffer, 0x0, BUFFER_LENGTH);
	} while (buffer_len > 0);

	// Close connection and free memory
	close(cliente->sockfd);

	free(cliente);
	pthread_detach(pthread_self());

	return NULL;
}

/*
 * Main execution of the server program of the simple protocol
 */
int main(void)
{

	/* Client and Server socket structures */
	struct sockaddr_in client, server;

	/* File deors of client and server */
	int serverfd;

	int i;
	for (i = 0; i < MAX_CLIENTS; i++)
	{
		clients[i] = NULL;
	}

	ctx_en =  EVP_CIPHER_CTX_new();
    ctx_de = EVP_CIPHER_CTX_new();
    if (aes_init((unsigned char*)key, keysize, (unsigned char *)&salt, ctx_en, ctx_de)) {
        printf("Couldn't initialize AES cipher\n");
        return -1;
    }

	fprintf(stdout, "Starting server\n");

	/* Creates a IPv4 socket */
	serverfd = socket(AF_INET, SOCK_STREAM, 0);
	if (serverfd == -1)
	{
		perror("Can't create the server socket:");
		return EXIT_FAILURE;
	}
	fprintf(stdout, "Server socket created with fd: %d\n", serverfd);

	/* Defines the server socket properties */
	server.sin_family = AF_INET;
	server.sin_port = htons(PORT);
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	memset(server.sin_zero, 0x0, 8);

	/* Handle the error of the port already in use */
	int yes = 1;
	if (setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR,
				   &yes, sizeof(int)) == -1)
	{
		perror("Socket options error:");
		return EXIT_FAILURE;
	}

	/* bind the socket to a port */
	if (bind(serverfd, (struct sockaddr *)&server, sizeof(server)) == -1)
	{
		perror("Socket bind error:");
		return EXIT_FAILURE;
	}

	/* Starts to wait connections from clients */
	if (listen(serverfd, 1) == -1)
	{
		perror("Listen error:");
		return EXIT_FAILURE;
	}
	fprintf(stdout, "Listening on port %d\n", PORT);

	printf("\n***** Starting the chatroom *****\n\n");

	/* Communicates with the client until bye message come */
	while (1)
	{
		socklen_t client_len = sizeof(client);
		pthread_t thread_id;
		int client_conn = 0;

		if ((client_conn = accept(serverfd,
								  (struct sockaddr *)&client, &client_len)) == -1)
		{
			perror("Accept error:");
			return EXIT_FAILURE;
		}

		for (i = 0; i < MAX_CLIENTS; i++)
		{
			if (clients[i] == NULL)
			{
				uid++;
				client_t *new_client = (client_t *)malloc(sizeof(client_t));
				new_client->address = client;
				new_client->sockfd = client_conn;
				new_client->uid = uid;
				add_client(new_client, i);
				break;
			}
		}
		if (i >= 5)
		{
			printf("Number max of clients reached\n");
		}
		else
		{
			pthread_create(&thread_id, NULL, &client_handler, (void *)clients[i]);
		}
	}

	/* Close the local socket */
	close(serverfd);

	printf("Connection closed\n\n");

	EVP_CIPHER_CTX_free(ctx_en);
    EVP_CIPHER_CTX_free(ctx_de);

	return EXIT_SUCCESS;
}

void add_client(client_t *c, int pos)
{
	if (clients[pos] == NULL)
	{
		// Add Client
		clients[pos] = c;
	}
}

void send_message_client(char *buffer, client_t *c)
{
	int i;
	for (i = 0; i < MAX_CLIENTS; i++)
	{
		if (clients[i] != NULL)
		{
			if (clients[i]->uid != c->uid)
			{
				write(clients[i]->sockfd, buffer, strlen(buffer));
			}
		}
	}
}