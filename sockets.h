#ifndef SOCKETS_TCP_H
#define SOCKETS_TCP_H

/* * Descomente a linha abaixo se for compilar no Windows (CodeBlocks/MinGW).
 * No Linux/macOS, deixe comentado.
 */
//#define WIN 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef WIN
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#endif

// --- Constantes ---
#define TAM_MENSAGEM 255         /* Mensagem de maior tamanho */
#define PORTA_SERVIDOR_TCP 9999  /* Porta padrão */
#define MAXPENDING 5             /* Número máximo de requisições para conexão pendentes */

typedef struct{
    char nome[16]; // 15 para o nome e um para o NULL
    char IP[16]; // 15 para endereço IP e um para o NULL
    char porta[6]; // 5 para a porta e um para o NULL
} usuario;
//usuario usuarios[10]; // sendo o número máximo de usuários

// --- Protótipos das Funções ---
int criar_socket(int porta);
int conectar_com_servidor(int sock, char *IP, int porta);
int aceitar_conexao(int sock);
int enviar_mensagem(char *mensagem, int sock);
int receber_mensagem(char *mensagem, int sock);
int socket_receber_mensagem(char *mensagem, int sock);
int socket_enviar_mensagem(char *mensagem, char *IP, int PORTA);

#endif // SOCKETS_TCP_H
