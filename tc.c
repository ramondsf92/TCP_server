// Cliente do chat TCP.
// Exemplo de chamada: ./tc NomeUsuario IPUsuario PortaUsuario IPServidor
#include "sockets.h"
#include <bits/pthreadtypes.h>
#include <stdio.h>
#include <signal.h>
#include <pthread.h>
#include <string.h>
#include <termios.h>

/* Variáveis globais e mutexes */

char NOME_USUARIO[TAM_MENSAGEM];
char IP_USUARIO[TAM_MENSAGEM];
char cPORTA_USUARIO[TAM_MENSAGEM];
int  PORTA_USUARIO;
char IP_SERVIDOR[TAM_MENSAGEM];
char destinatario[TAM_MENSAGEM];

usuario usuarios[10];          
int qtd_usuarios = 0;
pthread_mutex_t m_r;        

#define MAX_HISTORICO 5
char historico[MAX_HISTORICO][TAM_MENSAGEM];
int  hist_count = 0;           /* quantas posições preenchidas (0..MAX_HISTORICO) */
int  hist_next  = 0;           /* próxima posição a escrever (circular) */
pthread_mutex_t m_hist;        

/* Refresh da tela */
struct termios termios_original;   /* estado original do terminal, p/ restaurar */
int  raw_ativo = 0;                /* 1 se o modo raw foi ativado (stdin é TTY) */
pthread_mutex_t m_tela;            /* dono da tela E do buffer de input */
char linha[TAM_MENSAGEM];          /* o que o usuário está digitando agora */
int  linha_len = 0;

#define PROMPT "Digite (destinatario|mensagem): "

/*  Protocolo (parsing)  */

/* Formata um frame de chat recebido (D/B) numa linha legível para o histórico:
   "Remetente -> voce: texto" (direta) ou "Remetente -> todos: texto" (broadcast). */
void formatar_recebida(const char *frame, char *saida, size_t n)
{
    char tmp[TAM_MENSAGEM];
    char tipo = frame[0];

    strncpy(tmp, frame + 4, sizeof(tmp) - 1);   /* não mexe no frame original */
    tmp[sizeof(tmp) - 1] = '\0';

    char *remetente = strtok(tmp, "|");
    char *texto     = strtok(NULL, "|");
    if(remetente == NULL) remetente = "?";
    if(texto == NULL)     texto = "";

    if(tipo == 'D')
        snprintf(saida, n, "%s -> voce: %s", remetente, texto);
    else if(tipo == 'B')
        snprintf(saida, n, "%s -> todos: %s", remetente, texto);
    else
        snprintf(saida, n, "%s", frame);        /* tipo desconhecido: mantém cru */
}

/*  Histórico (buffer circular)  */

void adicionar_historico(const char *msg)
{
    pthread_mutex_lock(&m_hist);

    strncpy(historico[hist_next], msg, TAM_MENSAGEM - 1);
    historico[hist_next][TAM_MENSAGEM - 1] = '\0';

    hist_next = (hist_next + 1) % MAX_HISTORICO;
    if(hist_count < MAX_HISTORICO)
        hist_count++;

    pthread_mutex_unlock(&m_hist);
}

/*  Tela / UI  */


void redesenhar_input(void)
{
    printf("\r\033[2K%s%.*s", PROMPT, linha_len, linha);
    fflush(stdout);
}

void desenhar_tela(void)
{
    int i;
    usuario lista[10];
    int n;
    char hist_local[MAX_HISTORICO][TAM_MENSAGEM];
    int hc, inicio;

    pthread_mutex_lock(&m_tela);

    pthread_mutex_lock(&m_r);
    n = qtd_usuarios;
    for(i = 0; i < n && i < 10; i++)
        lista[i] = usuarios[i];
    pthread_mutex_unlock(&m_r);

    pthread_mutex_lock(&m_hist);
    hc = hist_count;
    inicio = (hist_next - hist_count + MAX_HISTORICO) % MAX_HISTORICO;
    for(i = 0; i < hc; i++)
        strcpy(hist_local[i], historico[(inicio + i) % MAX_HISTORICO]);
    pthread_mutex_unlock(&m_hist);

    printf("\033[2J\033[H");
    printf("=== CHAT (voce: %s) ===\n", NOME_USUARIO);

    printf("\nUsuarios online (%d):\n", n);
    for(i = 0; i < n; i++)
        printf("  - %-15s %s:%s\n", lista[i].nome, lista[i].IP, lista[i].porta);

    printf("\nUltimas mensagens:\n");
    for(i = 0; i < hc; i++)
        printf("  %s\n", hist_local[i]);

    printf("\nComandos: <nome>|<msg> = direta  |  broadcast|<msg> = todos  |  Ctrl+C = sair\n");
    printf("%s%.*s", PROMPT, linha_len, linha);
    fflush(stdout);

    pthread_mutex_unlock(&m_tela);
}

/*  Sinais / saída limpa  */

/* Sinaliza pedido de encerramento (setado pelo handler de SIGINT). */
volatile sig_atomic_t pediu_encerrar = 0;

/* Handler de SIGINT (Ctrl+C): apenas marca a intenção de sair. */
void tratar_sigint(int sig)
{
    (void)sig;
    pediu_encerrar = 1;
}

/* Saída limpa: avisa o servidor com S (logout), libera recursos e encerra.
   Chamada tanto no EOF do stdin quanto após Ctrl+C. */
void encerrar_cliente(void)
{
    /* Restaura o terminal (echo/canônico) antes de qualquer saída */
    if(raw_ativo)
        tcsetattr(STDIN_FILENO, TCSANOW, &termios_original);

    char payload[TAM_MENSAGEM];
    char mensagem[TAM_MENSAGEM];
    snprintf(payload, sizeof(payload), "%s|", NOME_USUARIO);
    montar_mensagem(mensagem, sizeof(mensagem), 'S', payload);

    printf("\nEncerrando: avisando o servidor (logout)...\n");
    fflush(stdout);
    socket_enviar_mensagem(mensagem, IP_SERVIDOR, PORTA_SERVIDOR_TCP);

    pthread_mutex_destroy(&m_r);
    pthread_mutex_destroy(&m_hist);
    pthread_mutex_destroy(&m_tela);

    exit(0);
}

/*  Envio de mensagens  */

int registro(void)
{
    char payload[TAM_MENSAGEM];
    char mensagem[TAM_MENSAGEM];

    snprintf(payload, sizeof(payload), "%s|%s|%s|", NOME_USUARIO, IP_USUARIO, cPORTA_USUARIO);
    montar_mensagem(mensagem, sizeof(mensagem), 'R', payload);

    return socket_enviar_mensagem(mensagem, IP_SERVIDOR, PORTA_SERVIDOR_TCP);
}

void enviar_msgdireta(char *destinatario, char *texto)
{
    char payload[TAM_MENSAGEM];
    char mensagem[TAM_MENSAGEM];

    snprintf(payload, sizeof(payload), "%s|%s|", NOME_USUARIO, texto);
    montar_mensagem(mensagem, sizeof(mensagem), 'D', payload);

    for(int i = 0; i < qtd_usuarios; i++)
    {
        if(strcmp(destinatario, usuarios[i].nome) == 0)
        {
            socket_enviar_mensagem(mensagem, usuarios[i].IP, atoi(usuarios[i].porta));
            return;
        }
    }
}

void enviar_msgbroadcast(char *texto)
{
    char payload[TAM_MENSAGEM];
    char mensagem[TAM_MENSAGEM];

    snprintf(payload, sizeof(payload), "%s|%s|", NOME_USUARIO, texto);
    montar_mensagem(mensagem, sizeof(mensagem), 'B', payload);

    for(int i = 0; i < qtd_usuarios; i++)
    {
        if(strcmp(usuarios[i].nome, NOME_USUARIO) == 0)
            continue;
        socket_enviar_mensagem(mensagem, usuarios[i].IP, atoi(usuarios[i].porta));
    }
}


void processar_entrada(char *entrada)
{
    char *separador = strchr(entrada, '|');
    if(separador == NULL)
        return;                 /* formato inválido: ignora */
    *separador = '\0';

    strcpy(destinatario, entrada);

    char texto[TAM_MENSAGEM];
    strcpy(texto, separador + 1);
    texto[strcspn(texto, "\r")] = '\0';

    if(strcmp(destinatario, "broadcast") == 0)
        enviar_msgbroadcast(texto);
    else
        enviar_msgdireta(destinatario, texto);

    char saida[TAM_MENSAGEM];
    snprintf(saida, sizeof(saida), "Voce -> %s: %s", destinatario, texto);
    adicionar_historico(saida);
}


void *receber(void *param)
{
    int sock = criar_socket(PORTA_USUARIO);
    char mensagem[TAM_MENSAGEM];

    for(;;)
    {
        memset(mensagem, 0, TAM_MENSAGEM);
        int status = socket_receber_mensagem(mensagem, sock);
        if(status != 200)
            continue;

        if(mensagem[0] == 'L')          
        {
            pthread_mutex_lock(&m_r);
            qtd_usuarios = 0;

            char dados[TAM_MENSAGEM];
            strcpy(dados, &mensagem[4]);

            char *token = strtok(dados, "|");
            while(token != NULL)
            {
                strcpy(usuarios[qtd_usuarios].nome, token);

                token = strtok(NULL, "|");
                if(token == NULL) break;
                strcpy(usuarios[qtd_usuarios].IP, token);

                token = strtok(NULL, "|");
                if(token == NULL) break;
                strcpy(usuarios[qtd_usuarios].porta, token);
                qtd_usuarios++;

                token = strtok(NULL, "|");
            }
            pthread_mutex_unlock(&m_r);

            desenhar_tela();            /* evento: a lista mudou */
        }
        else if(mensagem[0] != 'A')     /* mensagem de chat (ignora ACK 'A') */
        {
            char copia[TAM_MENSAGEM];
            formatar_recebida(mensagem, copia, sizeof(copia));
            adicionar_historico(copia);

            desenhar_tela();            /* evento: mensagem recebida */
        }
    }
}

void menu()
{
    int status = registro();
    if(status != 200)
    {
        printf("Nao foi possivel conctar com o servidor\n");
        return;
    }

    pthread_t t_receber;

    pthread_mutex_init(&m_r,NULL);
    pthread_mutex_init(&m_hist,NULL);
    pthread_mutex_init(&m_tela,NULL);

#ifndef WIN
    /* Bloqueia SIGINT antes de criar a thread: ela herda a máscara, então o
       Ctrl+C é sempre entregue à thread principal (a que está no read). */
    sigset_t set, oldset;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    pthread_sigmask(SIG_BLOCK, &set, &oldset);
#endif

    if(pthread_create(&t_receber, NULL, receber, NULL))
    {
        printf("\nERRO: criando thread.\n");
        return;
    }

#ifndef WIN
    /* A thread principal volta a aceitar SIGINT e instala o handler.
       sa_flags = 0 (sem SA_RESTART) para que o Ctrl+C interrompa o read(). */
    pthread_sigmask(SIG_SETMASK, &oldset, NULL);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = tratar_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    /* Ativa o modo raw: leitura char-a-char, sem echo (ecoamos manualmente),
       mantendo ISIG para o Ctrl+C continuar gerando SIGINT. */
    if(tcgetattr(STDIN_FILENO, &termios_original) == 0)
    {
        struct termios raw = termios_original;
        raw.c_lflag &= ~(ICANON | ECHO);
        raw.c_cc[VMIN]  = 1;
        raw.c_cc[VTIME] = 0;
        if(tcsetattr(STDIN_FILENO, TCSANOW, &raw) == 0)
            raw_ativo = 1;
    }
#endif

    desenhar_tela(); 

    for(;;)
    {
        char c;
        int r = read(STDIN_FILENO, &c, 1);

        if(r < 0)
        {
            if(pediu_encerrar)         /* CTRL+C */
                encerrar_cliente();
            continue;                   
        }
        if(r == 0 || c == 4)            /* EOF */
            encerrar_cliente();

        if(c == '\n' || c == '\r')
        {
            char entrada[TAM_MENSAGEM];

            pthread_mutex_lock(&m_tela);
            memcpy(entrada, linha, linha_len);
            entrada[linha_len] = '\0';
            linha_len = 0;
            pthread_mutex_unlock(&m_tela);

            processar_entrada(entrada);

            desenhar_tela();
        }
        else if(c == 127 || c == 8) 
        {
            pthread_mutex_lock(&m_tela);
            if(linha_len > 0)
                linha_len--;
            redesenhar_input();
            pthread_mutex_unlock(&m_tela);
        }
        else if((unsigned char)c >= 32 && linha_len < TAM_MENSAGEM - 1)
        {
            pthread_mutex_lock(&m_tela);
            linha[linha_len++] = c;
            redesenhar_input();
            pthread_mutex_unlock(&m_tela);
        }
    }
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

    if(argc != 5)    /* Testa se o número de parâmetros está correto */
    {
        printf("Uso: %s NOME_USUARIO IP_USUARIO PORTA_USUARIO IP_SERVIDOR\n", argv[0]);
        return(1);
    }

    memset(NOME_USUARIO, 0, TAM_MENSAGEM);
    strcpy(NOME_USUARIO, argv[1]);
    memset(IP_USUARIO, 0, TAM_MENSAGEM);
    strcpy(IP_USUARIO, argv[2]);
    memset(cPORTA_USUARIO, 0, TAM_MENSAGEM);
    strcpy(cPORTA_USUARIO, argv[3]);
    PORTA_USUARIO = atoi(cPORTA_USUARIO);
    memset(IP_SERVIDOR, 0, TAM_MENSAGEM);
    strcpy(IP_SERVIDOR, argv[4]);

    menu();

    return(0);
}
