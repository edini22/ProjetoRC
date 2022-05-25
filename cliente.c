// operations_terminal {endereço do servidor} {PORTO_BOLSA}
// ./operations_terminal 127.0.0.1 9000
// Server<->Cliente TCP

// bvl/stock_bvl_1/10

#include <arpa/inet.h>
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
#define PORTO1 9876
#define PORTO2 9875
int fd;
struct sockaddr_in multi1;
int sock_multi1;
struct sockaddr_in multi2;
int sock_multi2;
int multi_len1;
int multi_len2;
int subs[2]; // varivel para saber quais as subscricoes
struct ip_mreq mreq[2];

void erro(char *msg);

int login(int fd) {
    char buffer[BUF_SIZE];

    // Login: Username:
    read(fd, buffer, BUF_SIZE);
    printf("%s", buffer);
    if (!strcmp(buffer, "Nao existem usuarios registados!") || !strcmp(buffer, "Já estao 5 utilizadores!")) {
        return -1;
    }
    fflush(stdout);

    // Send username
    memset(buffer, 0, BUF_SIZE);
    scanf("%s", buffer);
    write(fd, buffer, BUF_SIZE);
    fflush(stdout);

    // Password:
    memset(buffer, 0, BUF_SIZE);
    read(fd, buffer, BUF_SIZE);
    printf("%s", buffer);
    fflush(stdout);

    // Send password
    memset(buffer, 0, BUF_SIZE);
    scanf("%s", buffer);
    write(fd, buffer, BUF_SIZE);
    fflush(stdout);

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
    char teste[BUF_SIZE * 4];
    while (1) {
        memset(teste, 0, BUF_SIZE * 4);
        if (subs[0] == 1) {
            if (recvfrom(sock_multi1, teste, BUF_SIZE * 4, 0, (struct sockaddr *)&multi1, (socklen_t *)&multi_len1) < 0) {
                printf("Erro a receber\n");
            }
            printf("%s\n", teste);
        }
        if (subs[1] == 1) {
            memset(teste, 0, BUF_SIZE * 4);
            recvfrom(sock_multi2, teste, BUF_SIZE * 4, 0, (struct sockaddr *)&multi2, (socklen_t *)&multi_len2);
            printf("%s\n", teste);
        }
    }
}

int main(int argc, char **argv) {

    struct sockaddr_in addr;
    struct hostent *hostPtr;

    if (argc != 3) {
        erro("Parametros mal especificados. Exemplo:\nstock_server {PORTO_BOLSA} {PORTO_CONFIG} {ficheiro configuração}");
        exit(-1);
    }

    // leitura dos parametros --------------------------------------------------------------------------------
    char *ponteiro;
    char endereco[50];
    char buffer[BUF_SIZE];

    strcpy(endereco, argv[1]);
    int PORTO_BOLSA = (int)strtol(argv[2], &ponteiro, 10);

    // Conexao TCP ----------------------------------------------------------------------------------------
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

    // Sockets Multicast ------------------------------------------------------------------------------------
    multi_len1 = sizeof(multi1);
    multi_len2 = sizeof(multi2);
    int multicastTTL = 255;
    subs[0] = 0;
    subs[1] = 0;

    if ((sock_multi1 = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        erro("na funcao socket(multicast)");
    }
    if (setsockopt(sock_multi1, SOL_SOCKET, SO_REUSEADDR, (void *)&multicastTTL, sizeof(multicastTTL)) < 0) {
        erro("funcao socket opt");
    }

    bzero((char *)&addr, sizeof(addr));
    multi1.sin_family = AF_INET;
    multi1.sin_addr.s_addr = htonl(INADDR_ANY);
    multi1.sin_port = htons(PORTO1); //  FIXME: verificar se e para usar o mesmo porto ou outro

    if ((sock_multi2 = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        erro("na funcao socket(multicast)");
    }
    if (setsockopt(sock_multi2, SOL_SOCKET, SO_REUSEADDR, (void *)&multicastTTL, sizeof(multicastTTL)) < 0) {
        erro("funcao socket opt");
    }
    bzero((char *)&addr, sizeof(addr));
    multi2.sin_family = AF_INET;
    multi2.sin_addr.s_addr = htonl(INADDR_ANY);
    multi2.sin_port = htons(PORTO2); //  FIXME: verificar se e para usar o mesmo porto ou outro

    if (bind(sock_multi1, (struct sockaddr *)&multi1, sizeof(multi1)) < 0) {
        erro("bind (multi1)");
    }
    if (bind(sock_multi2, (struct sockaddr *)&multi2, sizeof(multi2)) < 0) {
        erro("bind (multi2)");
    }

    // =#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#

    if (login(fd) == -1) {
        close(fd);
        exit(0);
    } else {
        // Receber do servidor os mercados que pode acede
        memset(buffer, 0, BUF_SIZE * 2);
        read(fd, buffer, BUF_SIZE * 2);
        printf("\n%s", buffer);

        // Mostrar Menu
        int escolha = 0;
        pthread_t feed_atualizacoes;
        int toggle = 0;
        while (1) {
            printf("\n--MENU--\n--1 Subscrever as cotacoes de um mercado\n--2 Comprar uma acao\n--3 Vender uma acao\n--4 Ligar/Desligar feed de atualizacoes do mercado\n--5 Ver carteira de acoes e o saldo\n--6 Sair\n");
            scanf("%d", &escolha);

            switch (escolha) {
            case 1: // Subscrever cotacao
                write(fd, "escolha1", 10);
                // Mostrar os mercados que tem acesso para poder subscrever
                memset(buffer, 0, BUF_SIZE);
                read(fd, buffer, BUF_SIZE);
                printf("%s", buffer);
                if (strcmp(buffer, "Nao tem acesso a nenhum mercado pelo que nao pode subscrever a nenhuma cotacao\n")) {
                    // Escolher mercado que pretende subscrever
                    char num[2];
                    scanf("%s", num);
                    int n = atoi(num);
                    if (subs[n - 1] == 1) {
                        num[0] = '9';
                    }
                    write(fd, num, 2);
                    // Resposta do server
                    memset(buffer, 0, BUF_SIZE);
                    read(fd, buffer, BUF_SIZE);
                    // Numero invalido
                    if (!strcmp(buffer, "O numero nao e valido\n") || !strcmp(buffer, "Ja se subscreveu este canal\n")) {
                        printf("%s", buffer);
                        // Receber o endereco do grupo multicast
                    } else {
                        printf("%s", buffer);
                        // Ler endereco do multicast
                        memset(buffer, 0, BUF_SIZE);
                        read(fd, buffer, BUF_SIZE);
                        printf("Endereco: %s\n", buffer);

                        // TODO: verificar se pode subscrever a mais mercados, i.e., if subs >= shared_memory->user.num_mercados;
                        if (!strcmp(buffer, "239.0.0.1")) {
                            printf("ENtrou!\n");
                            // colocar o endereco que se recebe na struct
                            mreq[0].imr_multiaddr.s_addr = inet_addr(buffer);
                            mreq[0].imr_interface.s_addr = htonl(INADDR_ANY);

                            if (setsockopt(sock_multi1, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq[0], sizeof(mreq[0])) < 0) {
                                erro("setsockopt mreq");
                            }

                            subs[0] = 1;
                        } else if (!strcmp(buffer, "239.0.0.2")) {

                            mreq[0].imr_multiaddr.s_addr = inet_addr(buffer);
                            mreq[0].imr_interface.s_addr = htonl(INADDR_ANY);

                            if (setsockopt(sock_multi2, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq[0], sizeof(mreq[0])) < 0) {
                                erro("setsockopt mreq");
                            }

                            subs[1] = 1;
                        }
                    }
                }

                break;

            case 2: // Comprar
                write(fd, "escolha2", 10);
                // Ver acoes que tem acesso
                char compra[BUF_SIZE * 2];
                read(fd, compra, BUF_SIZE * 2);
                printf("%s", compra);
                // BUG: esta a ccomparar mal!!
                if (strcmp(compra, "Voce nao tem acoes nao tem acesso a nenhum mercado!\n")) {
                    // Enviar o mercado/acao/num
                    printf("Insira {nome do mercado}/{nome da acao}/{quantidade}/{Preço}:\n");
                    memset(buffer, 0, BUF_SIZE);
                    scanf("%s", buffer);
                    write(fd, buffer, BUF_SIZE);

                    // Reposta do servidor
                    memset(buffer, 0, BUF_SIZE);
                    read(fd, buffer, BUF_SIZE);
                    printf("%s", buffer);
                }

                break;

            case 3: // Vender
                write(fd, "escolha3", 10);
                // Ver as acoes que possui para vender
                char venda[BUF_SIZE * 2];
                memset(venda, 0, BUF_SIZE * 2);
                read(fd, venda, BUF_SIZE * 2);
                printf("%s", venda);
                int cond = strcmp(venda, "Voce nao tem acoes em nenhum mercado!\n");
                if (cond != 0) {
                    // Enviar o mercado/acao/num
                    printf("Insira {nome do mercado}/{nome da acao}/{quantidade}/{Preço}:\n");
                    memset(buffer, 0, BUF_SIZE);
                    scanf("%s", buffer);
                    write(fd, buffer, BUF_SIZE);

                    // Reposta do servidor
                    memset(buffer, 0, BUF_SIZE);
                    read(fd, buffer, BUF_SIZE);
                    printf("%s", buffer);
                }

                break;

            case 4: // Ligar/desligar feed
                write(fd, "escolha4", 10);
                if (toggle == 0) {
                    toggle = 1;
                    pthread_create(&feed_atualizacoes, NULL, mostra_feed, NULL);
                } else {
                    toggle = 0;
                    pthread_cancel(feed_atualizacoes);
                }

                break;

            case 5: // Carteira
                write(fd, "escolha5", 10);
                // Mostrar informcacoes da carteira
                char carteira[BUF_SIZE * 2];
                memset(buffer, 0, BUF_SIZE);
                memset(carteira, 0, BUF_SIZE * 2);
                printf("espera!%s\n", carteira);
                read(fd, carteira, BUF_SIZE * 2);
                printf("%s", carteira);
                break;

            case 6: // Sair
                write(fd, "escolha6", 10);
                if (toggle == 1) {
                    pthread_cancel(feed_atualizacoes);
                }
                if (subs[0] == 1) {
                    if (setsockopt(sock_multi1, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq[0], sizeof(mreq[0])) < 0) {
                        erro("setsockopt mreq1");
                    }
                } else if (subs[1] == 1) {
                    if (setsockopt(sock_multi2, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq[1], sizeof(mreq[1])) < 0) {
                        erro("setsockopt mreq2");
                    }
                }
                close(fd);
                close(sock_multi1);
                close(sock_multi2);
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
