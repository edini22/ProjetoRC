// operations_terminal {endereço do servidor} {PORTO_BOLSA}
// ./operations_terminal 127.0.0.1 9000
// Server<->Cliente TCP

#include <netdb.h>
#include <netinet/in.h>
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
    fflush(stdout);
    memset(buffer, 0, BUF_SIZE);

    // Send username
    scanf("%s", buffer);
    write(fd, buffer, BUF_SIZE);
    fflush(stdout);
    memset(buffer, 0, BUF_SIZE);

    // Password:
    read(fd, buffer, BUF_SIZE);
    printf("%s", buffer);
    fflush(stdout);
    memset(buffer, 0, BUF_SIZE);

    // Send password
    scanf("%s", buffer);
    write(fd, buffer, BUF_SIZE);
    fflush(stdout);
    memset(buffer, 0, BUF_SIZE);

    // Verify login
    char erro_msgs[2][100] = {"\nO Username nao existe", "\nPassword incorreta"};
    read(fd, buffer, BUF_SIZE);
    if (!strcmp(buffer, erro_msgs[0]) || !strcmp(buffer, erro_msgs[1])) {
        printf("Username ou password errada\n");
        return -1;
    } else {
        printf("Login efetuado com sucesso!\n");
    }

    return 0;
}

int main(int argc, char **argv) {

    struct sockaddr_in addr;
    struct hostent *hostPtr;

    if (argc != 3) {
        erro("Parametros mal especificados. Exemplo:\nstock_server {PORTO_BOLSA} {PORTO_CONFIG} {ficheiro configuração}");
    }

    // leitura dos parametros

    char *ponteiro;
    char endereco[50];
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
        exit(0);
    } else {
        // Receber do servidor os mercados que pode acede
        char buffer[BUF_SIZE];
        read(fd, buffer, BUF_SIZE);
        printf("%s", buffer);

        // Mostrar Menu
        int escolha = 0;
        printf("--MENU--");
        printf("--1 Subscrever as cotacoes de um mercado");
        printf("--2 Comprar uma acao");
        printf("--3 Vender uma acao");
        printf("--4 Ligar/Desligar feed de atualizacoes do mercado");
        printf("--5 Ver carteira de acoes e o saldo");
        printf("--6 Sair");

        while (1) {
            switch (escolha) {
            case 1:
                write(fd, "escolha1", 10);

                break;
            case 2:
                write(fd, "escolha2", 10);

                break;
            case 3:
                write(fd, "escolha3", 10);
                

                break;
            case 4:
                write(fd, "escolha4", 10);

                break;
            case 5:
                // Mostrar informcacoes da carteira
                write(fd, "escolha5", 10);
                read(fd, buffer, BUF_SIZE);
                printf("%s", buffer);
                break;
            case 6:
                write(fd, "escolha6", 10);

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
