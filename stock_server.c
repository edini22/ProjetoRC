// stock_server {PORTO_BOLSA} {PORTO_CONFIG} {ficheiro configuração}
// ./stock_server 9000 9001 config_file.txt
// Server<->Cliente TCP
// Server<->Consola_Admin UDP
// TCP - nc -v localhost 9000
// UDP - nc -u -4 localhost 9001

#include "func.h"

pid_t pid;
pid_t pid_refresh;

void sigint(int signum) {
    terminar(shm_id);
    close(fd);
    close(s);
    kill(pid, SIGKILL);
    kill(pid_refresh, SIGKILL);
}

int main(int argc, char **argv) {
    // Variaveis TCP
    int client_fd;
    struct sockaddr_in addr, client_addr;
    int client_addr_size;

    // Variaveis UDP
    int s;
    struct sockaddr_in admin_addr;
    int recv_len, send_len;
    socklen_t slen = sizeof(admin_addr);

    pid_t cpid[2];

    if (argc != 4) {
        erro("stock_server {PORTO_BOLSA} {PORTO_CONFIG} {ficheiro configuração}");
        exit(-1);
    }

    // Leitura dos parametros
    short SERVER_PORT = (short)atoi(argv[1]);
    short SERVER_CONFIG = (short)atoi(argv[2]);
    char path[200];
    strcpy(path, argv[3]);

    // TCP ---------------------------------------------------------------------------------
    bzero((void *)&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(SERVER_PORT);

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        erro("na funcao socket (TCP)");
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        erro("na funcao bind (TCP)");
    if (listen(fd, 11) < 0)
        erro("na funcao listen (TCP)");

    client_addr_size = sizeof(client_addr);

    // UDP ---------------------------------------------------------------------------------
    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        erro("na funcao socket (UDP)");
    }

    admin_addr.sin_family = AF_INET;
    admin_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    admin_addr.sin_port = htons(SERVER_CONFIG);

    if (bind(s, (struct sockaddr *)&admin_addr, sizeof(admin_addr)) == -1) {
        erro("na funcao bind (UDP)");
    }

    // MULTICAST ---------------------------------------------------------------------------
    struct sockaddr_in multi1;
    int sock_multi1;
    struct sockaddr_in multi2;
    int sock_multi2;
    int multicastTTL = 255;

    if ((sock_multi1 = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        erro("na funcao socket(multicast)");
    }
    if (setsockopt(sock_multi1, IPPROTO_IP, IP_MULTICAST_TTL, (void *)&multicastTTL, sizeof(multicastTTL)) < 0) {
        erro("funcao socket opt");
    }
    bzero((char *)&addr, sizeof(addr));
    multi1.sin_family = AF_INET;
    multi1.sin_addr.s_addr = htonl(INADDR_ANY);
    multi1.sin_port = htons(PORTO1);
    multi1.sin_addr.s_addr = inet_addr(GROUP1);
    

    if ((sock_multi2 = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        erro("na funcao socket(multicast)");
    }
    if (setsockopt(sock_multi2, IPPROTO_IP, IP_MULTICAST_TTL, (void *)&multicastTTL, sizeof(multicastTTL)) < 0) {
        erro("funcao socket opt");
    }
    bzero((char *)&addr, sizeof(addr));
    multi2.sin_family = AF_INET;
    multi2.sin_addr.s_addr = htonl(INADDR_ANY);
    multi2.sin_port = htons(PORTO2);
    multi2.sin_addr.s_addr = inet_addr(GROUP2);

    //--------------------------------------------------------------------------------------
    // Ignore signal
    // signal(SIGINT, SIG_IGN);

    // Open shared memory
    shm_id = shmget(IPC_PRIVATE, sizeof(SM), IPC_CREAT | IPC_EXCL | 0700);

    // Attach
    shared_memory = shmat(shm_id, NULL, 0);

    shared_memory->mercados[0].num_acoes = 0;
    shared_memory->mercados[1].num_acoes = 0;
    shared_memory->num_mercados = 0;
    shared_memory->clientes_atuais = 0;
    for (int i = 0; i < 5; i++) {
        shared_memory->atuais[i].ocupado = false;
    }
    for (int i = 0; i < 10; i++) {
        shared_memory->users[i].ocupado = false;
    }

    // Ler e verificar se o config_file esta correto
    if (config(path) == -1) {
        printf("A terminar programa.\n");
        terminar(shm_id);
        exit(0);
    }

    sem_unlink("SEM_COMPRAS");
    sem_unlink("SEM_USERS");
    // sem_unlink("MUTEX_LOGIN");
    shared_memory->sem_compras = sem_open("SEM_COMPRAS", O_CREAT | O_EXCL, 0700, 1);
    shared_memory->sem_users = sem_open("SEM_USERS", O_CREAT | O_EXCL, 0700, 1);
    // shared_memory->mutex_login = sem_open("MUTEX_LOGIN", O_CREAT | O_EXCL, 0700, 0);
    // int i = 0;
    shared_memory->refresh_time = 2;

    printf("A iniciar o servidor...\n");
    // // Ignore Signals in this process
    // signal(SIGTSTP, SIG_IGN);
    // signal(SIGINT, SIG_IGN);
    // signal(SIGHUP, SIG_IGN);  // Hung up the process
    // signal(SIGQUIT, SIG_IGN); // Quit the process

    // REFRESH =============================================================================
    if ((pid_refresh = fork()) == 0) {
        shared_memory->refresh_pid = getpid();
        char mercado1[BUF_SIZE * 4];
        char mercado2[BUF_SIZE * 4];
        snprintf(mercado1, BUF_SIZE, "Multicast1:\n");
        snprintf(mercado2, BUF_SIZE, "Multicast2:\n");
        int r;
        srand(time(NULL));
        while (1) {
            sleep(shared_memory->refresh_time);
            sem_wait(shared_memory->sem_compras);
            for (int m = 0; m < shared_memory->num_mercados; m++) {
                memset(mercado1, 0, BUF_SIZE);
                memset(mercado2, 0, BUF_SIZE);
                for (int a = 0; a < shared_memory->mercados[m].num_acoes; a++) {
                    if (shared_memory->mercados[m].acoes[a].preco >= 0.02) {
                        r = rand() % 2;
                        if (r == 0)
                            shared_memory->mercados[m].acoes[a].preco -= 0.01;
                        else
                            shared_memory->mercados[m].acoes[a].preco += 0.01;
                    } else {
                        shared_memory->mercados[m].acoes[a].preco += 0.01;
                    }
                    if (shared_memory->mercados[m].acoes[a].n_acoes == 100) {
                        shared_memory->mercados[m].acoes[a].n_acoes -= 10;
                    } else if (shared_memory->mercados[m].acoes[a].n_acoes <= 10) {
                        shared_memory->mercados[m].acoes[a].n_acoes += 10;
                    } else {
                        r = rand() % 2;
                        if (r == 0)
                            shared_memory->mercados[m].acoes[a].n_acoes -= 10;
                        else
                            shared_memory->mercados[m].acoes[a].n_acoes += 10;
                    }
                    if (m == 0) {
                        char aux[BUF_SIZE * 2];
                        snprintf(aux, BUF_SIZE * 2, "Mercado %s, Acao %s, Preco %f, N Acoes %d\n", shared_memory->mercados[m].nome, shared_memory->mercados[m].acoes[a].nome, shared_memory->mercados[m].acoes[a].preco, shared_memory->mercados[m].acoes[a].n_acoes);
                        strcat(mercado1, aux);
                    } else {
                        char aux[BUF_SIZE * 2];
                        snprintf(aux, BUF_SIZE * 2, "Mercado %s, Acao %s, Preco %f, N Acoes %d\n", shared_memory->mercados[m].nome, shared_memory->mercados[m].acoes[a].nome, shared_memory->mercados[m].acoes[a].preco, shared_memory->mercados[m].acoes[a].n_acoes);
                        strcat(mercado2, aux);
                    }
                }
                int nbytes = sendto(sock_multi1, mercado1, strlen(mercado1), 0, (struct sockaddr *)&multi1, sizeof(multi1));
                if (nbytes < 0) {
                    perror("sendto");
                    return 1;
                }
                sendto(sock_multi2, mercado2, strlen(mercado2), 0, (struct sockaddr *)&multi2, sizeof(multi2));
            }
            sem_post(shared_memory->sem_compras);
        }
    }

    // TCP =================================================================================
    if ((pid = fork()) == 0) {
        while (shared_memory->clientes_atuais < 5) {
            while (waitpid(-1, NULL, WNOHANG) > 0)
                ;
            // wait for new connection TCP
            client_fd = accept(fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_size);
            printf("Client trying to connect...\n");

            pid_t cpid = fork();
            if (cpid == 0) {

                // Escolhas feitas pelo user
                int id = process_client(client_fd);

                remove_cpid(client_fd);
                if (id != -1)
                    printf("Cliente n%d deslogado (%s)\n", shared_memory->clientes_atuais, shared_memory->users[id].nome);

                close(fd);
                close(client_fd);
                exit(0);
            }
        }

        exit(0);
    }

    // printf("mercado1: %s\n", shared_memory->mercados[0].nome);
    // printf("mercado2: %s\n", shared_memory->mercados[1].nome);
    // printf("Num mercados: %d\n", shared_memory->num_mercados);

    // Catch signals by main process
    // signal(SIGTSTP, SIGTSTP_HANDLER);
    // signal(SIGINT, SIGINT_HANDLER);

    // Redirect signal
    // signal(SIGINT, sigint);

    // UDP =================================================================================
    char buffer[BUF_SIZE];
    while (login_admin(s) == -1)
        ;

    struct sockaddr_in admin_outra;
    char menu[] = "\nMENU:\nADD_USER {username} {password} {bolsas a que tem acesso} {saldo}\nDEL {username}\nLIST\nREFRESH {segundos}\nQUIT\nQUIT_SERVER\n\n";

    while (1) { // MENU ADMIN

        // Esperara pelo \n do cliente depois do login
        recvfrom(s, buffer, BUF_SIZE, 0, (struct sockaddr *)&admin_outra, (socklen_t *)&slen);

        // Mostrar o menu
        sendto(s, menu, strlen(menu), 0, (struct sockaddr *)&admin_outra, slen);

        // Comando usado pelo Admin
        memset(buffer, 0, BUF_SIZE);
        if (recvfrom(s, buffer, BUF_SIZE, 0, (struct sockaddr *)&admin_outra, (socklen_t *)&slen) == -1) {
            erro("funcao recvfrom");
        }

        // printf("Buffer: %s\n", buffer);
        int condicao = 0;
        // Separar os argumentos dos comandos
        if (strlen(buffer) > 1)
            buffer[strlen(buffer) - 1] = '\0';
        int count = 0;
        char buffer2[BUF_SIZE];
        strcpy(buffer2, buffer);
        char buffer3[BUF_SIZE];
        strcpy(buffer3, buffer);
        char *token = strtok(buffer, " ");
        while (token != NULL) {
            // printf("%s\n", token);
            count++;
            token = strtok(NULL, " ");
        }
        // printf("Count: %d\n", count);

        if (!strcmp(buffer, "ADD_USER")) { // ------------------------------------------
            // printf("Entrou dentro do add user\n");

            if (count < 4 || count > 6) {
                memset(buffer, 0, BUF_SIZE);
                snprintf(buffer, BUF_SIZE, "Numero de parametros errado. Clique enter para continuar\n");
                sendto(s, buffer, strlen(buffer), 0, (struct sockaddr *)&admin_outra, slen);
                printf("%s", buffer);

            } else {
                int index = 0;
                char *toke = strtok(buffer3, " ");
                for (int n = 0; n < 2; n++) {
                    // printf("%s\n", toke);
                    if (n == 1) {
                        sem_wait(shared_memory->sem_users);
                        for (int i = 0; i < 10; i++) {
                            if (shared_memory->users[i].ocupado == true) {
                                char aux[500];
                                strcpy(aux, shared_memory->users[i].nome);
                                if (!strcmp(aux, toke)) {
                                    index = i;
                                    condicao++;
                                    break;
                                }
                            }
                        }
                        sem_post(shared_memory->sem_users);
                    }
                    toke = strtok(NULL, " ");
                }
                // printf("\ncondiçao == %d\n", condicao);//TODO:VERIFICAR SE JA TEM O MERCADO A ADICIONAR! para nao resetar os mercados!
                if (condicao == 1) {
                    char *tok = strtok(buffer2, " ");
                    sem_wait(shared_memory->sem_users);
                    for (int n = 0; n < count; n++) {
                        if (n == 1) {
                            shared_memory->users[index].num_mercados = 0;
                            for (int m = 0; m < count; m++) {
                                shared_memory->users[index].mercados[m].acesso = false;
                            }
                        } else if (n == 2) {
                            strcpy(shared_memory->users[index].password, tok);
                        } else if ((n == 3 && count == 5) || (n == 3 && count == 6) || (n == 4 && count == 6)) {
                            if (shared_memory->num_mercados != 0) {

                                for (int j = 0; j < shared_memory->num_mercados; j++) {

                                    int count = 0;

                                    if (!strcmp(shared_memory->mercados[j].nome, tok)) {
                                        count++;
                                        for (int m = 0; m < 2; m++) {
                                            if (shared_memory->users[index].mercados[m].ocupado == true && !strcmp(shared_memory->users[index].mercados[m].nome, tok)) {
                                                shared_memory->users[index].mercados[m].acesso = true;
                                                shared_memory->users[index].num_mercados++;
                                                count = -1;
                                                break;
                                            }
                                        }
                                        if (count == -1) {
                                            break;
                                        } else if (count == 1) {
                                            for (int i = 0; i < 2; i++) {
                                                if (shared_memory->users[index].mercados[i].ocupado == false) {
                                                    strcpy(shared_memory->users[index].mercados[i].nome, tok);
                                                    shared_memory->users[index].num_mercados++;
                                                    shared_memory->users[index].mercados[i].ocupado = true;
                                                    shared_memory->users[index].mercados[i].acesso = true;

                                                    if (!strcmp(shared_memory->users[index].mercados[i].nome, shared_memory->mercados[0].nome)) {
                                                        for (int a = 0; a < shared_memory->mercados[0].num_acoes; a++) {
                                                            shared_memory->users[index].mercados[i].acao[a].n_acoes = 0;
                                                            strcpy(shared_memory->users[index].mercados[i].acao[a].nome, shared_memory->mercados[0].acoes[a].nome);
                                                        }
                                                        shared_memory->users[index].mercados[i].num_acoes = shared_memory->mercados[0].num_acoes;
                                                        shared_memory->users[index].mercados[i].n_acoes_comp_mercado = 0;
                                                    } else if (!strcmp(shared_memory->users[index].mercados[i].nome, shared_memory->mercados[1].nome)) {
                                                        for (int a = 0; a < shared_memory->mercados[1].num_acoes; a++) {
                                                            shared_memory->users[index].mercados[i].acao[a].n_acoes = 0;
                                                            strcpy(shared_memory->users[index].mercados[i].acao[a].nome, shared_memory->mercados[1].acoes[a].nome);
                                                        }
                                                        shared_memory->users[index].mercados[i].num_acoes = shared_memory->mercados[0].num_acoes;
                                                        shared_memory->users[index].mercados[i].n_acoes_comp_mercado = 0;
                                                    }
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        } else if ((n == 3 && count == 4) || (n == 4 && count == 5) || (n == 5 && count == 6)) {
                            shared_memory->users[index].saldo = (float)atof(tok);
                        }
                        tok = strtok(NULL, " ");
                    }
                    sem_post(shared_memory->sem_users);
                    memset(buffer, 0, BUF_SIZE);
                    snprintf(buffer, BUF_SIZE, "user %s atualizado! Clique enter para continuar\n", shared_memory->users[index].nome);
                    sendto(s, buffer, strlen(buffer), 0, (struct sockaddr *)&admin_outra, slen);

                } else {
                    if (shared_memory->num_utilizadores < 10) {
                        for (int i = 0; i < 10; i++) { // colocar novo user!!!
                            if (shared_memory->users[i].ocupado == false) {

                                char *tok = strtok(buffer2, " ");
                                sem_wait(shared_memory->sem_users);
                                for (int n = 0; n < count; n++) {
                                    if (n == 1) {
                                        for (int m = 0; m < 2; m++) {
                                            shared_memory->users[i].mercados[m].ocupado = false;
                                            shared_memory->users[i].mercados[m].acesso = false;
                                        }
                                        shared_memory->users[i].num_mercados = 0;
                                        shared_memory->users[i].num_acoes_compradas = 0;
                                        strcpy(shared_memory->users[i].nome, tok);
                                    } else if (n == 2) {
                                        strcpy(shared_memory->users[i].password, tok);
                                    } else if ((n == 3 && count == 5) || (n == 3 && count == 6) || (n == 4 && count == 6)) {
                                        if (shared_memory->num_mercados != 0) {
                                            for (int j = 0; j < shared_memory->num_mercados; j++) {
                                                char aux[BUF_SIZE];
                                                strcpy(aux, shared_memory->mercados[j].nome);
                                                if (!strcmp(aux, tok)) {
                                                    strcpy(shared_memory->users[i].mercados[shared_memory->users[i].num_mercados].nome, aux);
                                                    shared_memory->users[i].mercados[shared_memory->users[i].num_mercados].ocupado = true;
                                                    shared_memory->users[i].mercados[shared_memory->users[i].num_mercados].acesso = true;
                                                    shared_memory->users[i].num_mercados++;
                                                    // TODO: adicionar acoes e inicializar variaveis!!
                                                    if (!strcmp(shared_memory->users[i].mercados[shared_memory->users[i].num_mercados].nome, shared_memory->mercados[0].nome)) {
                                                        for (int a = 0; a < shared_memory->mercados[0].num_acoes; a++) {
                                                            shared_memory->users[i].mercados[shared_memory->users[i].num_mercados].acao[a].n_acoes = 0;
                                                            strcpy(shared_memory->users[i].mercados[shared_memory->users[i].num_mercados].acao[a].nome, shared_memory->mercados[0].acoes[a].nome);
                                                        }
                                                        shared_memory->users[i].mercados[shared_memory->users[i].num_mercados].num_acoes = shared_memory->mercados[0].num_acoes;
                                                        shared_memory->users[i].mercados[shared_memory->users[i].num_mercados].n_acoes_comp_mercado = 0;

                                                    } else if (!strcmp(shared_memory->users[i].mercados[shared_memory->users[i].num_mercados].nome, shared_memory->mercados[1].nome)) {
                                                        for (int a = 0; a < shared_memory->mercados[1].num_acoes; a++) {
                                                            shared_memory->users[i].mercados[shared_memory->users[i].num_mercados].acao[a].n_acoes = 0;
                                                            strcpy(shared_memory->users[i].mercados[shared_memory->users[i].num_mercados].acao[a].nome, shared_memory->mercados[1].acoes[a].nome);
                                                        }
                                                        shared_memory->users[i].mercados[shared_memory->users[i].num_mercados].num_acoes = shared_memory->mercados[0].num_acoes;
                                                        shared_memory->users[i].mercados[shared_memory->users[i].num_mercados].n_acoes_comp_mercado = 0;
                                                    }

                                                    break;
                                                }
                                            }
                                        }
                                    } else if ((n == 3 || count == 4) || (n == 4 && count == 5) || (n == 5 && count == 6)) {
                                        shared_memory->users[i].saldo = (float)atof(tok);
                                        shared_memory->users[i].ocupado = true;
                                        shared_memory->num_utilizadores++;
                                    }
                                    tok = strtok(NULL, " ");
                                }
                                sem_post(shared_memory->sem_users);
                                memset(buffer, 0, BUF_SIZE);
                                snprintf(buffer, BUF_SIZE, "user %s adicionado! Clique enter para continuar\n", shared_memory->users[i].nome);
                                sendto(s, buffer, strlen(buffer), 0, (struct sockaddr *)&admin_outra, slen);
                                break;
                            }
                        }

                    } else {
                        memset(buffer, 0, BUF_SIZE);
                        snprintf(buffer, BUF_SIZE, "Atingiu o numero maximo de utilizadores. Clique enter para continuar\n");
                        sendto(s, buffer, strlen(buffer), 0, (struct sockaddr *)&admin_outra, slen);
                        // printf("%s", buffer);
                    }
                }
            }
        }

        else if (!strcmp(buffer, "DEL")) { // PROCURAR NO ARRAY E COLOCAR O BOOL A FALSE!!
            if (shared_memory->num_utilizadores == 0) {
                memset(buffer, 0, BUF_SIZE);
                snprintf(buffer, BUF_SIZE, "Nao existem utilizadores registados! Clique enter para continuar\n");
                sendto(s, buffer, strlen(buffer), 0, (struct sockaddr *)&admin_outra, slen);
                // printf("%s", buffer);
            } else if (count != 2) {
                memset(buffer, 0, BUF_SIZE);
                snprintf(buffer, BUF_SIZE, "Numero de parametros errado! Clique enter para continuar\n");
                sendto(s, buffer, strlen(buffer), 0, (struct sockaddr *)&admin_outra, slen);
                // printf("%s", buffer);
            } else {
                bool existe = false;
                char *tok = strtok(buffer2, " ");
                sem_wait(shared_memory->sem_users);
                for (int n = 0; n < 2; n++) {
                    // printf("%s\n", tok);
                    if (n == 1) {
                        for (int i = 0; i < 10; i++) {
                            if (shared_memory->users[i].ocupado == true) {
                                char aux[500];
                                strcpy(aux, shared_memory->users[i].nome);
                                if (!strcmp(aux, tok)) {
                                    existe = true;
                                    shared_memory->users[i].ocupado = false;
                                    shared_memory->num_utilizadores--;
                                    memset(buffer, 0, BUF_SIZE);
                                    snprintf(buffer, BUF_SIZE, "user %s removido! Clique enter para continuar\n", aux);
                                    sendto(s, buffer, strlen(buffer), 0, (struct sockaddr *)&admin_outra, slen);
                                    // printf("%s", buffer);
                                    break;
                                }
                            }
                        }
                        if (!existe) {
                            memset(buffer, 0, BUF_SIZE);
                            snprintf(buffer, BUF_SIZE, "Nao existe nenhum user com esse nome registado! Clique enter para continuar\n");
                            sendto(s, buffer, strlen(buffer), 0, (struct sockaddr *)&admin_outra, slen);
                            // printf("%s", buffer);
                        }
                    }
                    tok = strtok(NULL, " ");
                }
                sem_post(shared_memory->sem_users);
            }
        } else if (!strcmp(buffer, "LIST")) {
            sem_wait(shared_memory->sem_users);
            if (shared_memory->num_utilizadores == 0) {
                sem_post(shared_memory->sem_users);
                memset(buffer, 0, BUF_SIZE);
                snprintf(buffer, BUF_SIZE, "Nao existem utilizadores registados! Clique enter para continuar\n");
                sendto(s, buffer, strlen(buffer), 0, (struct sockaddr *)&admin_outra, slen);
                // printf("%s", buffer);
            } else {
                char print[2048];
                memset(print, 0, BUF_SIZE);
                for (int i = 0; i < 10; i++) {
                    if (shared_memory->users[i].ocupado) {
                        char aux[1500];
                        memset(aux, 0, BUF_SIZE);
                        snprintf(aux, 1500, "\nNome: %s Password: %s Saldo: %4.2f ", shared_memory->users[i].nome, shared_memory->users[i].password, shared_memory->users[i].saldo);
                        char aux2[BUF_SIZE];
                        printf("mercados = %d\n", shared_memory->users[i].num_mercados);
                        //
                        if (shared_memory->users[i].num_mercados != 0) {
                            for (int m = 0; m < 2; m++) {
                                if (shared_memory->users[i].mercados[m].acesso == true) {
                                    memset(aux2, 0, BUF_SIZE);
                                    snprintf(aux2, BUF_SIZE, " %s ", shared_memory->users[i].mercados[m].nome);
                                    strcat(aux, aux2);
                                }
                            }
                        }
                        strcat(print, aux);
                    }
                }
                sem_post(shared_memory->sem_users);
                memset(buffer, 0, BUF_SIZE);
                sendto(s, print, strlen(print), 0, (struct sockaddr *)&admin_outra, slen);
            }
        } else if (!strcmp(buffer, "REFRESH")) { // ------------------------------------------
            if (count == 2) {
                char *tok = strtok(buffer2, " ");
                for (int n = 0; n < 2; n++) {
                    printf("%s\n", tok);
                    if (n == 1) {
                        shared_memory->refresh_time = (int)atoi(tok);
                    }
                    tok = strtok(NULL, " ");
                }
                memset(buffer, 0, BUF_SIZE);
                snprintf(buffer, BUF_SIZE, "Tempo do refresh alterado para %d! Clique enter para continuar\n", shared_memory->refresh_time);
                sendto(s, buffer, strlen(buffer), 0, (struct sockaddr *)&admin_outra, slen);
                // printf("%s", buffer);
            } else {
                memset(buffer, 0, BUF_SIZE);
                snprintf(buffer, BUF_SIZE, "Numero de parametros errado! Clique enter para continuar\n");
                sendto(s, buffer, strlen(buffer), 0, (struct sockaddr *)&admin_outra, slen);
                printf("%s", buffer);
            }
        } else if (!strcmp(buffer, "QUIT")) {
            memset(buffer, 0, BUF_SIZE);
            snprintf(buffer, BUF_SIZE, "O admin deslogado!\n");
            sendto(s, buffer, strlen(buffer), 0, (struct sockaddr *)&admin_outra, slen);
            printf("%s", buffer);
            while (login_admin(s) == -1) // FIXME: quando o admin sai, a consola do nc nao fecha pq fica a espera de login
                ;

        } else if (!strcmp(buffer, "QUIT_SERVER")) { // ------------------------------------------
            memset(buffer, 0, BUF_SIZE);
            snprintf(buffer, BUF_SIZE, "A encerrar o servidor...\n");
            sendto(s, buffer, strlen(buffer), 0, (struct sockaddr *)&admin_outra, slen);
            printf("%s", buffer);
            break;
        } else {
            memset(buffer, 0, BUF_SIZE);
            snprintf(buffer, BUF_SIZE, "Opcao invalida... Clique enter para continuar\n");
            sendto(s, buffer, strlen(buffer), 0, (struct sockaddr *)&admin_outra, slen);
            printf("%s", buffer);
        }
    }

    terminar(shm_id);

    // while (wait(NULL) != 1 || errno != ECHILD) {
    //     printf("wainted for a child to finish\n");
    // }

    // fechar os sockets no processo main!
    close(fd);
    close(s);
    close(sock_multi1);
    close(sock_multi2);
    kill(pid, SIGKILL);

    return 0;
}