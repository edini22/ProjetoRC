// stock_server {PORTO_BOLSA} {PORTO_CONFIG} {ficheiro configuração}
// Server<->Cliente TCP
// Server<->Consola_Admin UDP
// TCP - nc -v localhost 9000
// UDP - nc -u -4 localhost 9001

#include "func.h"

int shm_id;
SM *shared_memory;

int main(int argc, char **argv) {
    // Variaveis TCP
    int fd, client;
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

    // TCP -----------------------------------
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

    // UDP ---------------------------------------
    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        erro("na funcao socket (UDP)");
    }

    admin_addr.sin_family = AF_INET;
    admin_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    admin_addr.sin_port = htons(SERVER_CONFIG);

    if (bind(s, (struct sockaddr *)&admin_addr, sizeof(admin_addr)) == -1) {
        erro("na funcao bind (UDP)");
    }

    // Open shared memory
    shm_id = shmget(IPC_PRIVATE, sizeof(SM), IPC_CREAT | IPC_EXCL | 0700);

    // Attach
    shared_memory = shmat(shm_id, NULL, 0);

    shared_memory->mercados[0].num_acoes = 0;
    shared_memory->mercados[1].num_acoes = 0;
    shared_memory->num_mercados = 0;
    shared_memory->clientes_atuais = 0;
    for(int i = 0; i<5; i++){
        shared_memory->atuais[i].ocupado = false;
    }

    config(path, shared_memory);

    sem_unlink("MUTEX_COMPRAS");
    // sem_unlink("MUTEX_MENU");
    // sem_unlink("MUTEX_LOGIN");
    shared_memory->mutex_compras = sem_open("MUTEX_COMPRAS", O_CREAT | O_EXCL, 0700, 1);
    // shared_memory->mutex_menu = sem_open("MUTEX_MENU", O_CREAT | O_EXCL, 0700, 1);
    // shared_memory->mutex_login = sem_open("MUTEX_LOGIN", O_CREAT | O_EXCL, 0700, 0);
    // int i = 0;
    shared_memory->refresh_time = 2;

    printf("A iniciar o servidor...\n");

    // REFRESH
    pid_t pid_refresh;
    pid_refresh = fork();
    if (pid_refresh == 0) {
        while (1) {
            sem_wait(shared_memory->mutex_compras);
            // printf("\n------------------acao!---------------------\n");
            sleep(shared_memory->refresh_time);
            for (int m = 0; m < shared_memory->num_mercados; m++) {
                for (int a = 0; a < shared_memory->mercados[m].num_acoes; a++) {
                    if (shared_memory->mercados[m].acoes[a].preco_inicial >= 0.02) {
                        time_t t;
                        srand((unsigned)time(&t));
                        int r = rand() % 2;
                        if (r == 0)
                            shared_memory->mercados[m].acoes[a].preco_inicial -= 0.01;
                        else
                            shared_memory->mercados[m].acoes[a].preco_inicial += 0.01;
                    } else {
                        shared_memory->mercados[m].acoes[a].preco_inicial += 0.01;
                    }
                }
            }
            sem_post(shared_memory->mutex_compras);
        }
    }

    // TCP =================================================================================
    pid_t pid;
    pid = fork();
    if (pid == 0) {

        while (shared_memory->clientes_atuais < 5) {
            while (waitpid(-1, NULL, WNOHANG) > 0)
                ;
            // wait for new connection TCP
            client = accept(fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_size);
            printf("Client connected...\n");


            pid_t cpid = fork();
            if (cpid == 0) {
                if (login(client, shared_memory) == -1) {
                    close(fd);
                    close(client);
                    exit(0);
                } else {
                    // TODO: coisas que o user pode fazer
                    add_cpid(client, shared_memory);

                    printf("Cliente %d logado\n", shared_memory->clientes_atuais);

                    int sair = 0;
                    if(sair){
                        remove_cpid(cliente, shared_memory);
                        close(fd);
                        close(client);
                        exit(0);
                    }

                    
                }
            }
        }

        exit(0);
    }

    printf("mercado1: %s---\n", shared_memory->mercados[0].nome);
    printf("mercado2: %s----\n", shared_memory->mercados[1].nome);
    printf("Num mercados: %d\n", shared_memory->num_mercados);

    // UDP =================================================================================
    char buffer[BUF_SIZE];
    while (login_admin(s, shared_memory) == -1)
        ;

    struct sockaddr_in admin_outra;
    char menu[] = "\nMENU:\nADD_USER {username} {password} {bolsas a que tem acesso} {saldo}\nDEL {username}\nLIST\nREFRESH {segundos}\nQUIT\nQUIT_SERVER\n\n";

    while (1) { // MENU ADMIN

        // Esperara pelo \n do cliente depois do login
        recvfrom(s, buffer, BUF_SIZE, 0, (struct sockaddr *)&admin_outra, (socklen_t *)&slen);

        // Mostrar o menu
        sendto(s, menu, strlen(menu), 0, (struct sockaddr *)&admin_outra, slen);

        // Comando usado pelo Admin
        // printf("sendto\n");
        memset(buffer, 0, BUF_SIZE);
        if (recvfrom(s, buffer, BUF_SIZE, 0, (struct sockaddr *)&admin_outra, (socklen_t *)&slen) == -1) {
            erro("funcao recvfrom");
        }

        printf("Buffer: %s\n", buffer);
        // Separar os argumentos dos comandos
        if (strlen(buffer) > 1)
            buffer[strlen(buffer) - 1] = '\0';
        int count = 0;
        char buffer2[BUF_SIZE];
        strcpy(buffer2, buffer);
        char *token = strtok(buffer, " ");
        while (token != NULL) {
            printf("%s\n", token);
            count++;
            token = strtok(NULL, " ");
        }
        printf("Count: %d\n", count);

        if (!strcmp(buffer, "ADD_USER")) { // ------------------------------------------
            printf("Entrou dentro do add user\n");

            if (count <= 4 || count >= 6) {
                printf("Numero de parametros errado. Clique enter para continuar\n");
            } else {
                bool existe = false;
                // for (int i = 0; i < 10; i++) { // verificar se o user existe
                //     if (shared_memory->users[i].ocupado) {
                //         printf("1");
                //         char aux[BUF_SIZE];
                //         strcpy(aux, shared_memory->users[i].user.nome);
                //         char *tok = strtok(buffer2, " ");
                //         for (int n = 0; n < count; n++) {
                //             if (n == 1) {
                //                 if (!strcmp(tok, aux)) {
                //                     printf("O user existe:)\n");
                //                     existe = true;
                //                     shared_memory->users[i].ocupado = false;
                //                     shared_memory->num_utilizadores--;
                //                     printf("user %s removido!", aux);
                //                 }
                //             }
                //             tok = strtok(NULL, " ");
                //         }
                //         if(existe)break;
                //     }
                // }

                if (shared_memory->num_utilizadores < 10) {
                    if (!existe) {
                        for (int i = 0; i < 10; i++) { // colocar novo user!!!
                            if (shared_memory->users[i].ocupado == false) {

                                char *tok = strtok(buffer2, " ");
                                for (int n = 0; n < count; n++) {
                                    printf("%s\n", tok);
                                    if (n == 1) {
                                        shared_memory->users[i].user.num_mercados = 0;
                                        strcpy(shared_memory->users[i].user.nome, tok);
                                    } else if (n == 2) {
                                        strcpy(shared_memory->users[i].user.password, tok);
                                    } else if ((n == 3 && count == 5) || (n == 3 && count == 6) || (n == 4 && count == 6)) {
                                        if (shared_memory->num_mercados != 0) {
                                            printf("mercado !=0!!\n");
                                            for (int j = 0; j < shared_memory->num_mercados; j++) {
                                                printf("j = %d\n", j);
                                                char aux[BUF_SIZE];
                                                strcpy(aux, shared_memory->mercados[j].nome);
                                                printf("%s -> %d\n", aux, strcmp(aux, tok));
                                                if (!strcmp(aux, tok)) {
                                                    printf("add_mercado!!");
                                                    strcpy(shared_memory->users[i].user.mercados[shared_memory->users[i].user.num_mercados].nome, aux);
                                                    shared_memory->users[i].user.num_mercados++;
                                                }
                                            }
                                        }
                                    } else if ((n == 4 && count == 5) || (n == 5 && count == 6)) {
                                        shared_memory->users[i].user.saldo_inicial = (float)atof(tok);
                                        shared_memory->users[i].ocupado = true;
                                        shared_memory->num_utilizadores++;
                                    }
                                    tok = strtok(NULL, " ");
                                }
                                break;
                            }
                        }
                    }
                } else {
                    printf("Atingiu o numero maximo de utilizadores.\n");
                }
            }
        }

        else if (!strcmp(buffer, "DEL")) { // PROCURAR NO ARRAY E COLOCAR O BOOL A FALSE!!
            if (shared_memory->num_utilizadores == 0) {
                printf("Nao existem utilizadores registados!\n");
            } else {
                bool existe = false;
                char *tok = strtok(buffer2, " ");
                for (int n = 0; n < 2; n++) {
                    printf("%s\n", tok);
                    if (n == 1) {
                        for (int i = 0; i < 10; i++) {
                            if (shared_memory->users[i].ocupado == true) {
                                char aux[BUF_SIZE];
                                strcpy(aux, shared_memory->users[i].user.nome);
                                if (!strcmp(aux, tok)) { // se der erro, falta memset nas variaveis :)
                                    existe = true;
                                    shared_memory->users[i].ocupado = false;
                                    shared_memory->num_utilizadores--;
                                    printf("user %s removido!", aux);
                                    break;
                                }
                            }
                        }
                        if (!existe)
                            printf("Nao existe nenhum user com esse nome registado!\n");
                    }
                    tok = strtok(NULL, " ");
                }
            }
        } else if (!strcmp(buffer, "LIST")) { // TODO: SENDTO para o cliente!
            if (shared_memory->num_utilizadores == 0) {
                printf("Nao existem utilizadores registados!\n");
            } else {
                char print[BUF_SIZE];
                memset(print, 0, BUF_SIZE);
                for (int i = 0; i < 10; i++) {
                    if (shared_memory->users[i].ocupado == true) {
                        char aux[BUF_SIZE];
                        memset(aux, 0, BUF_SIZE);
                        snprintf(aux, BUF_SIZE, "Nome: %s Password: %s Saldo: %4.2f\n ", shared_memory->users[i].user.nome, shared_memory->users[i].user.password, shared_memory->users[i].user.saldo_inicial);
                        char aux2[BUF_SIZE];
                        if (shared_memory->users[i].user.num_mercados == 0) {
                            printf("mercados = %d\n", shared_memory->users[i].user.num_mercados);
                            snprintf(aux2, BUF_SIZE, "O user ainda nao tem mercados\n");
                            strcat(print, aux2);
                        } else {
                            snprintf(aux2, BUF_SIZE, "Mercados do user:\n");
                            strcat(aux, aux2);
                            for (int j = 0; i < shared_memory->users[i].user.num_mercados; j++) {
                                snprintf(aux2, BUF_SIZE, " %s \n", shared_memory->users[i].user.mercados[j].nome);
                                strcat(aux, aux2);
                            }
                        }
                        strcat(print, aux);
                    }
                }
                printf("%s", print); // debug
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
            }
        } else if (!strcmp(buffer, "QUIT")) { // FECHAR A LIGACAO, ACHO QUE É PRECISO FAZER UM NOVO FORK E UM RECVFROM
            close(s);

            if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
                erro("na funcao socket (UDP)");
            }

            admin_addr.sin_family = AF_INET;
            admin_addr.sin_addr.s_addr = htonl(INADDR_ANY);
            admin_addr.sin_port = htons(SERVER_CONFIG);

            if (bind(s, (struct sockaddr *)&admin_addr, sizeof(admin_addr)) == -1) {
                erro("na funcao bind (UDP)");
            }

            printf("O admin saiu\n");
        } else if (!strcmp(buffer, "QUIT_SERVER")) { // ------------------------------------------
            printf("A encerrar o servidor...\n");
            break;
        }
    }

    // int status;
    // wait(&status);

    // wait(&status);
    // if (WEXITSTATUS(status) == 1)
    //     kill(shared_memory->childs_pid[1], SIGKILL);
    // else
    //     kill(shared_memory->childs_pid[0], SIGKILL);

    // close(shared_memory->clients_fd[0]);
    // close(shared_memory->clients_fd[1]);

    terminar(shm_id, shared_memory);

    // while (wait(NULL) != 1 || errno != ECHILD) {
    //     printf("wainted for a child to finish\n");
    // }

    // fechar os sockets no processo main!
    close(fd);
    close(s);
    kill(pid, SIGKILL);
    kill(pid_refresh, SIGKILL);

    return 0;
}