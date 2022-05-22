// operations_terminal {endereço do servidor} {PORTO_BOLSA}
// ./operations_terminal 127.0.0.1 9000
// Server<->Cliente TCP

#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUF_SIZE 1024
int fd;

void erro(char *msg);

int login(int fd) {
    char buffer[BUF_SIZE];

    // Login: Username:
    read(fd, buffer, BUF_SIZE);
    printf("%s", buffer);
    // fflush(stdout);

    // Send username
    memset(buffer, 0, BUF_SIZE);
    scanf("%s", buffer);
    write(fd, buffer, BUF_SIZE);
    // fflush(stdout);

    // Password:
    memset(buffer, 0, BUF_SIZE);
    read(fd, buffer, BUF_SIZE);
    printf("%s", buffer);
    // fflush(stdout);

    // Send password
    memset(buffer, 0, BUF_SIZE);
    scanf("%s", buffer);
    write(fd, buffer, BUF_SIZE);
    // fflush(stdout);

    // Verify login
    char erro_msgs[2][100] = {"\nO Username nao existe", "\nPassword incorreta"};
    memset(buffer, 0, BUF_SIZE);
    read(fd, buffer, BUF_SIZE); //
    if (!strcmp(buffer, erro_msgs[0]) || !strcmp(buffer, erro_msgs[1])) {
        printf("Username ou password errada\n");
        return -1;
    } else {
        printf("Login efetuado com sucesso!\n");
    }

    return 0;
}

void *mostra_feed(void *args) {
    // TODO: funcoes de broadcast e cenas
    while(1){
        sleep(1);
        printf("feed\n");
    }
}

int main(int argc, char **argv) {

    struct sockaddr_in addr;
    struct hostent *hostPtr;

    if (argc != 3) {
        erro("Parametros mal especificados. Exemplo:\nstock_server {PORTO_BOLSA} {PORTO_CONFIG} {ficheiro configuração}");
        exit(-1);
    }

    // leitura dos parametros

    char *ponteiro;
    char endereco[50];
    char buffer[BUF_SIZE];

    strcpy(endereco, argv[1]);
    int PORTO_BOLSA = (int)strtol(argv[2], &ponteiro, 10);

    // Conexao TCP
    if ((hostPtr = gethostbyname(endereco)) == 0) {
        erro("Endereco errado");
    }
    bzero((void *)&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ((struct in_addr *)(hostPtr->h_addr))->s_addr;
    addr.sin_port = htons((short)PORTO_BOLSA);

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        erro("Socket");
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        erro("Connect");

    if (login(fd) == -1) {
        close(fd);
        exit(0);
    } else {
        // Receber do servidor os mercados que pode acede
        memset(buffer, 0, BUF_SIZE);
        read(fd, buffer, BUF_SIZE);
        printf("%s", buffer);

        // Mostrar Menu
        int escolha = 0;
        pthread_t feed_atualizacoes;
        int toggle = 0;
        while (1) {
            printf("--MENU--\n--1 Subscrever as cotacoes de um mercado\n--2 Comprar uma acao\n--3 Vender uma acao\n--4 Ligar/Desligar feed de atualizacoes do mercado\n--5 Ver carteira de acoes e o saldo\n--6 Sair\n");
            scanf("%d", &escolha);

            switch (escolha) {
            case 1:
                write(fd, "escolha1", 10);

                break;
            case 2:
                write(fd, "escolha2", 10);
                // Ver acoes que tem acesso
                memset(buffer, 0, BUF_SIZE);
                read(fd, buffer, BUF_SIZE);
                printf("%s", buffer);

                 // Enviar o mercado/acao/num
                printf("Insira {nome do mercado}/{nome da acao}/{quantidade}:\n");
                char compra[BUF_SIZE];
                scanf("%s", compra);
                write(fd, compra, BUF_SIZE);

                // Reposta do servidor
                memset(buffer, 0, BUF_SIZE);
                read(fd, buffer, BUF_SIZE);
                printf("%s", buffer);

                break;
            case 3:
                write(fd, "escolha3", 10);
                // Ver as acoes que possui
                memset(buffer, 0, BUF_SIZE);
                read(fd, buffer, BUF_SIZE);
                printf("%s", buffer);

                break;
            case 4:
                write(fd, "escolha4", 10);
                if (toggle == 0) {
                    toggle = 1;
                    pthread_create(&feed_atualizacoes, NULL, mostra_feed, NULL);
                } else {
                    toggle = 0;
                    pthread_cancel(feed_atualizacoes);
                }

                break;
            case 5:
                // Mostrar informcacoes da carteira
                write(fd, "escolha5", 10);
                memset(buffer, 0, BUF_SIZE);
                printf("espera!%s\n", buffer);
                read(fd, buffer, BUF_SIZE);
                printf("%s", buffer);
                break;
            case 6:
                write(fd, "escolha6", 10);
                close(fd);
                exit(0);
                break;
            }
        }

        exit(0);
    }

    return 0;
}

void erro(char *msg) {
    perror(msg);
    exit(1);
}
