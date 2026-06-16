#include"sockets.h"
#include <string.h>
int criar_socket(int porta)
{
    int sock;
    struct sockaddr_in endereco; /* Endereço Local */

    /* Criação do socket datagrama/UDP para recepção e envio de pacotes */
    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\nErro na criação do socket!\n");fflush(stdout);
        return(-1);
    }

    if (porta > 0)
    {
        /* Construção da estrutura de endereço local */
        memset(&endereco, 0, sizeof(endereco));       /* Zerar a estrutura */
        endereco.sin_family      = AF_INET;           /* Família de endereçamento da Internet */
        endereco.sin_addr.s_addr = htonl(INADDR_ANY); /* Qualquer interface de entrada */
        endereco.sin_port        = htons(porta);      /* Porta local */

        /* Instanciar o endereco local */
        if (bind(sock, (struct sockaddr *) &endereco, sizeof(endereco)) < 0)
        {
           printf("\nErro no bind()!\n");fflush(stdout);
           return(-1);
        }

        /* Indica que o socket escutara as conexões */
        if (listen(sock, MAXPENDING) < 0)
        {
           printf("\nErro no listen()!\n");fflush(stdout);
           return(-1);
        }

    }
    if (sock < 0)
    {
        printf("\nErro na criação do socket!\n");
        return(1);
    }


    return(sock);
}

int recv_all(int sock, char *buffer, int tamanho)
{
    int recebidos = 0;

    while(recebidos < tamanho)
    {
        int r = recv(sock,
                     buffer + recebidos,
                     tamanho - recebidos,
                     0);

        if(r <= 0)
            return -1;

        recebidos += r;
    }

    return 0;
}
int conectar_com_servidor(int sock,char *IP,int porta)
{
    struct sockaddr_in endereco; /* Endereço Local */

    /* Construção da estrutura de endereço do servidor */
    memset(&endereco, 0, sizeof(endereco));   /* Zerar a estrutura */
    endereco.sin_family      = AF_INET;       /* Família de endereçamento da Internet */
    endereco.sin_addr.s_addr = inet_addr(IP); /* Endereço IP do Servidor */
    endereco.sin_port        = htons(porta);  /* Porta do Servidor */

    /* Estabelecimento da conexão com o servidor de echo */
    if (connect(sock, (struct sockaddr *) &endereco, sizeof(endereco)) < 0)
    {
        printf("\nErro no connect()!\n");fflush(stdout);
        return(-1);
    }
    return(0);
}

int aceitar_conexao(int sock)
{
    int                socket_cliente;
    struct sockaddr_in endereco; /* Endereço Local */
    int                tamanho_endereco;

    /* Define o tamanho do endereço de recepção e envio */
    tamanho_endereco = sizeof(endereco);

    /* Aguarda pela conexão de um cliente */
    if ((socket_cliente = accept(sock, (struct sockaddr *) &endereco, &tamanho_endereco)) < 0)
    {
        //printf("\nErro no accept()!\n");fflush(stdout);
        return(0);
    }
    return(socket_cliente);
}


int enviar_mensagem(char *mensagem,int sock)
{
    
    /* Envia o conteúdo da mensagem para o cliente */
    if (send(sock, mensagem, strlen(mensagem), 0) != strlen(mensagem))
    {
        printf("\nErro no envio da mensagem\n");fflush(stdout);
        return(-1);
    }

    //printf("\nTCP Cliente: Enviei (%s)\n",mensagem);fflush(stdout);

    return(0);
}

int receber_mensagem(char *mensagem,int sock)
{
    /* Limpar o buffer da mensagem */
    memset((void *) mensagem,(int) NULL, (TAM_MENSAGEM));

    /* Espera pela recepção de alguma mensagem do cliente conectado*/
    char tipo;
    if (recv_all(sock, &tipo, 1) < 0)
    {
        printf("\nErro na recepção da mensagem\n");fflush(stdout);
        return(-1);
    }

    char valtam[4];
    memset((void *) valtam, 0, 4 * sizeof(char));

    if (recv_all(sock, valtam, 3) < 0)
    {
        printf("\nErro na recepção da mensagem\n");fflush(stdout);
        return(-1);
    }
    int tam = atoi(valtam);

    char texto[TAM_MENSAGEM];
    memset((void *) texto,(int) NULL, (TAM_MENSAGEM));
    // limpa a mensagem (memset) depois...
    if (tam > 0)
    {
        if(recv_all(sock, texto, tam) < 0)
        {
            return -1;
        }
    texto[tam] = '\0';
    }

    // ADIÇÃO
    mensagem[0] = tipo;
    mensagem[1] = '\0';
    strcat(mensagem, valtam);
    strcat(mensagem, texto);
    
    return(0);
}

int socket_enviar_mensagem(char *mensagem, char *IP, int PORTA)
{
    int sock;
    int resultado;

    sock = criar_socket(0);
    if (sock < 0)
    {
        printf("\nErro na criação do socket!\n");
        return(404);
    }

    resultado = conectar_com_servidor(sock,IP,PORTA);
    if (resultado < 0)
    {
        printf("\nErro na conexão com o servidor\n");
        return(404);
    }

    resultado = enviar_mensagem(mensagem,sock);
    if (resultado < 0)
    {
        printf("\nErro no envio da mensagem\n");
        return(500);
    }

    char resposta[TAM_MENSAGEM];
    memset(resposta, 0, TAM_MENSAGEM);
    resultado = receber_mensagem(resposta, sock);
    if (resultado < 0)
    {
        printf("\nErro no recebimento da mensagem\n");
        close(sock);
        return(500);
    }

    close(sock);

    if(resposta[0] == 'A')
    {
        return 200;
    }

    return 500;

    if(mensagem[0] == 'A')
    {
        return (200);
    }
    else 
    {
        return (500);
    }
}

int socket_receber_mensagem(char *mensagem, int sock)
{
    
    int socket_cliente;
    int resultado;

    /* Aguarda por uma conexão e a aceita criando o socket de contato com o cliente */
    socket_cliente = aceitar_conexao(sock);
    if (socket_cliente == 0)
    {
        //printf("\nErro na conexao do socket!\n");
        return(404);
    }

    /* Recebe a mensagem do cliente */
    resultado = receber_mensagem(mensagem, socket_cliente);
    if (resultado < 0)
    {
        resultado = enviar_mensagem("N000", socket_cliente);
        return(500);
    }

    resultado = enviar_mensagem("A000", socket_cliente);
    if (resultado < 0)
    {
        printf("\nErro no envio da mensagem\n");
        return(500);
    }

    close(socket_cliente);    /* Fecha o socket do cliente */

    return (200);
}

