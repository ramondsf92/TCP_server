//Exemplo de chamada: ./cli NomeUsurio IPUsurio PortaUsurio IPServidor
#include"sockets.h"
#include <bits/pthreadtypes.h>
#include <stdio.h>

#include <signal.h>
#include <pthread.h>
#include <string.h>

char NOME_USUARIO[TAM_MENSAGEM]; 
char IP_USUARIO[TAM_MENSAGEM];          
char cPORTA_USUARIO[TAM_MENSAGEM];
int PORTA_USUARIO;
char IP_SERVIDOR[TAM_MENSAGEM];
char destinatario[TAM_MENSAGEM];

pthread_mutex_t m_r;

void enviar_msgbroadcast(char *texto);
void enviar_msgdireta(char *destinatario, char *texto);

void *receber(void *param)
{
    int sock = criar_socket(PORTA_USUARIO);
    char mensagem[TAM_MENSAGEM];
    
    memset((void *) mensagem,(int) NULL, sizeof(mensagem));
    
    for(;;)
    {
        int status = socket_receber_mensagem(mensagem, sock);
        if(status == 200)
        {
            pthread_mutex_lock(&m_r);
            printf("\nTCP cliente: (%s)\n",mensagem);fflush(stdout);
            pthread_mutex_unlock(&m_r);
        }
    }
}

int registro()
{
    char mensagem[TAM_MENSAGEM];
    memset((void *) mensagem,(int) NULL, sizeof(mensagem));
    strcat(mensagem, "R000");
    strcat(mensagem, NOME_USUARIO);
    strcat(mensagem, "|");
    strcat(mensagem, IP_USUARIO);
    strcat(mensagem, "|");
    strcat(mensagem, cPORTA_USUARIO);
    strcat(mensagem, "|");
    int tam = strlen(mensagem) - 4;
    //mensagem[1] = '0' + (tam / 100) % 10; 
    //mensagem[2] = '0' + (tam / 10) % 10; 
    //mensagem[3] = '0' + (tam / 1) % 10; 

    printf("%s\n", mensagem);
    int status = socket_enviar_mensagem(mensagem, IP_SERVIDOR, PORTA_SERVIDOR_TCP);

    return status;
}

// Função broadcast
void enviar_msgbroadcast(char *texto){

    // Tratamento da mensagem, seguindo os mesmos passos da direta para definir o protocolo (B000|Nome|Texto)
    char mensagem[TAM_MENSAGEM];
    
    memset((void *) mensagem, (int) NULL, TAM_MENSAGEM);
    
    strcat(mensagem, "B000");
    strcat(mensagem, NOME_USUARIO); 
    strcat(mensagem, "|");
    strcat(mensagem, texto);
    strcat(mensagem, "|");
    
    int tam = strlen(mensagem) - 4;
    mensagem[1] = (char)('0' + (tam / 100) % 10);
    mensagem[2] = (char)('0' + (tam / 10) % 10);
    mensagem[3] = (char)('0' + (tam / 1) % 10);

    // A alterar com a lista de usuários, onde praticamente é um for no socket de mensagens
    // Alterar para ler a lista de usuários, contar quantos tem e fazer a execução
    int lista_portas[] = {6000, 6001, 6002, 6003};
    int total_usuarios = 4;

    for(int i = 0; i < total_usuarios; i++)
    {
        if(lista_portas[i] != PORTA_USUARIO)
        {
            // Evitar sobrescrita na mensagem no broadcast
            char copia[TAM_MENSAGEM];
            memset(copia, 0, TAM_MENSAGEM);
            strcpy(copia, mensagem);
            
            socket_enviar_mensagem(copia, "127.0.0.1", lista_portas[i]);
           
        }
    }
}

// Para envio é (nome)|(mensagem) para ativar a mensagem direta
void enviar_msgdireta(char *destinatario, char *texto){
    char mensagem[TAM_MENSAGEM];
    memset(mensagem, 0, TAM_MENSAGEM);
    strcat(mensagem, "D000");
    strcat(mensagem, NOME_USUARIO);
    strcat(mensagem, "|");
    strcat(mensagem, texto);
    strcat(mensagem, "|");
    int tam = strlen(mensagem) - 4;
    mensagem[1] = (char)('0' + (tam / 100) % 10);
    mensagem[2] = (char)('0' + (tam / 10) % 10);
    mensagem[3] = (char)('0' + (tam / 1) % 10);

    /* A alterar quando chegar a lista, onde trocará os nomes pré definidos pela lista, em que 
    ao informar o nome, procurará na lista a sua porta*/
    if(strcmp(destinatario, "maria") == 0)
    {
        socket_enviar_mensagem(mensagem,"127.0.0.1",6001);
    }
    else if(strcmp(destinatario, "pedro") == 0)
    {
        socket_enviar_mensagem(mensagem,"127.0.0.1",6000);
    }
    else if(strcmp(destinatario, "bob") == 0)
    {
        socket_enviar_mensagem(mensagem,"127.0.0.1",6002);
    }
     else if(strcmp(destinatario, "hugo") == 0)
    {
        socket_enviar_mensagem(mensagem,"127.0.0.1",6003);
    }
    else if(strcmp(destinatario, "broadcast") == 0)
    {
        enviar_msgbroadcast(texto);
    }
}

void menu()
{
   int status = registro(); 

    if(status != 200)
    {
        printf("Nao foi possivel conectar com o servidor\n");
        return ;
    }

    pthread_t t_receber;

    pthread_mutex_init(&m_r,NULL);

    // chamada das pthreads
    if(0 || pthread_create(&t_receber,NULL,receber,NULL)) {
        printf("\nERRO: criando thread.\n");
        return;
    }

    char texto[TAM_MENSAGEM];


    for(;;)
    {
    
        char entrada[TAM_MENSAGEM];

        printf("\nDigite (destinatario|mensagem): ");
        fgets(entrada, TAM_MENSAGEM, stdin);
        entrada[strcspn(entrada, "\n")] = '\0';
        char *separador = strchr(entrada, '|');

        if(separador == NULL)
        {
            printf("Formato invalido!\n");
            continue;
        }

        *separador = '\0';

        strcpy(destinatario, entrada);
        strcpy(texto, separador + 1);

        char mensagem[TAM_MENSAGEM];
        memset((void *) mensagem,(int) NULL, TAM_MENSAGEM);
        
        // Altera dinamicamente o prefixo do protocolo para 'B' no servidor caso seja um broadcast
        if(strcmp(destinatario, "broadcast") == 0) {
            strcat(mensagem, "B000");
        } else {
            strcat(mensagem, "D000");
        }
        
        strcat(mensagem, NOME_USUARIO);
        strcat(mensagem, "|");
        strcat(mensagem, texto);
        strcat(mensagem, "|");
        int tam = strlen(mensagem) - 4;
        mensagem[1] = (char)('0' + (tam / 100) % 10); 
        mensagem[2] = (char)('0' + (tam / 10) % 10); 
        mensagem[3] = (char)('0' + (tam / 1) % 10); 


        enviar_msgdireta(destinatario, texto); // envia mensagem direta ao cliente informado
        socket_enviar_mensagem(mensagem, IP_SERVIDOR, PORTA_SERVIDOR_TCP); // envia ao servidor o protocolo informado
    

    }

    pthread_kill(t_receber, 0);
    pthread_mutex_destroy(&m_r);
}


int main(int argc, char *argv[])
{
#ifdef WIN
    WORD wPackedValues;
    WSADATA  SocketInfo;
    int      nLastError,
	         nVersionMinor = 1,
	         nVersionMajor = 1;
    wPackedValues = (WORD)(((WORD)nVersionMinor)<< 8)|(WORD)nVersionMajor;
    nLastError = WSAStartup(wPackedValues, &SocketInfo);
#endif

    if (argc != 5)    /* Testa se o nmero de parmetros est correto */
    {
        printf("Uso: %s NOME_USUARIO IP_USUARIO PORTA_USUARIO IP_SERVIDOR\n", argv[0]);
        return(1);
    }
    memset((void *) NOME_USUARIO ,(int) NULL,TAM_MENSAGEM);
    strcpy(NOME_USUARIO ,argv[1]); 
    memset((void *) IP_USUARIO ,(int) NULL,TAM_MENSAGEM);
    strcpy(IP_USUARIO ,argv[2]);
    memset((void *) cPORTA_USUARIO ,(int) NULL,TAM_MENSAGEM);
    strcpy(cPORTA_USUARIO ,argv[3]);
    PORTA_USUARIO = atoi(cPORTA_USUARIO);
    memset((void *) IP_SERVIDOR ,(int) NULL,TAM_MENSAGEM);
    strcpy(IP_SERVIDOR ,argv[4]);

    menu();

    return(0);
}