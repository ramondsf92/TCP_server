#include "sockets.h"
#include "string.h"
#include <stdlib.h>

#define MAX_CLIENTES 10
#define TAM_MENSAGEM 1024

// typedef struct {
//     char nome[MAX_NOME];
//     char ip[16];
//     int porta;
// } Cliente;

int qtd_clientes = 0;
usuario clientes[MAX_CLIENTES];


int adicionar_cliente(char *nome, char *ip, char *porta) {
    int i;

    for(i = 0; i < qtd_clientes; i++) {
        if(strcmp(clientes[i].nome, nome) == 0){
            printf("Cliente já cadastrado. Escolha outro nome");
            return 0;
        }
    }

    if(qtd_clientes >= MAX_CLIENTES) {
        printf("Quantidade máxima de clientes atingida. Aguarde abrir uma vaga e tente novamente.");
        return -1;
    }

    strncpy(clientes[qtd_clientes].nome, nome, 15);
    clientes[qtd_clientes].nome[15] = '\0';

    strncpy(clientes[qtd_clientes].IP, ip, sizeof(clientes[qtd_clientes].IP) - 1);
    clientes[qtd_clientes].IP[sizeof(clientes[qtd_clientes].IP) - 1] = '\0';

    strncpy(clientes[qtd_clientes].porta,porta,5);
    clientes[qtd_clientes].porta[5] = '\0';

    qtd_clientes++;

    return 1;
}

int remover_cliente(const char *nome)
{
    int i, j;

    for(i = 0; i < qtd_clientes; i++)
    {
        if(strcmp(clientes[i].nome, nome) == 0)
        {
            for(j = i; j < qtd_clientes - 1; j++)
            {
                clientes[j] = clientes[j + 1];
            }

            qtd_clientes--;
            return 1;
        }
    }

    return 0;
}

void enviar_lista_usuarios()
{
    char payload[TAM_MENSAGEM];
    char mensagem[TAM_MENSAGEM];
    int i;

    payload[0] = '\0';

    for(i = 0; i < qtd_clientes; i++)
    {
        char registro[64];

        sprintf(registro,
                "%s|%s|%d",
                clientes[i].nome,
                clientes[i].IP,
                clientes[i].porta);

        strcat(payload, registro);

        if(i < qtd_clientes - 1)
            strcat(payload, "|");
    }

    sprintf(mensagem,
            "L%03d%s",
            (int)strlen(payload),
            payload);

    printf("Broadcast: %s\n", mensagem);

    for(i = 0; i < qtd_clientes; i++)
    {
        int resultado = socket_enviar_mensagem(
            mensagem,
            clientes[i].IP,
            atoi(clientes[i].porta)
        );
    }
}

void menu()
{
    int sock = criar_socket(PORTA_SERVIDOR_TCP);
    char mensagem[TAM_MENSAGEM];
    
    for(;;)
    {
        socket_receber_mensagem(mensagem, sock);
        printf("\nTCP server: (%s)\n",mensagem);fflush(stdout);
        enviar_lista_usuarios();
    }
}

int main()
{
    // int sock;
#ifdef WIN
    WORD wPackedValues;
    WSADATA  SocketInfo;
    int      nLastError,
	         nVersionMinor = 1,
	         nVersionMajor = 1;
    wPackedValues = (WORD)(((WORD)nVersionMinor)<< 8)|(WORD)nVersionMajor;
    nLastError = WSAStartup(wPackedValues, &SocketInfo);
#endif

    menu();

    return (0);
}
