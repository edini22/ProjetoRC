// stock_server {PORTO_BOLSA} {PORTO_CONFIG} {ficheiro configuração}
// Server<->Cliente TCP
// Server<->Consola_Admin UDP

#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h>
#include <netinet/in.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUF_SIZE 1024

typedef struct {
    char nome[50];
    char password[50];
    float saldo_inicial;
    int clients_fd;
} user;

typedef struct {
    char nome[50];
    float preco_inicial;
} acao;

typedef struct {
    char nome[50];
    acao acoes[3];
    int num_acoes;
} mercado;

typedef struct {
    char admin[2][30]; //[0] AdminName [1]AdminPassword
    int num_utilizadores;
    user users[10];
    mercado mercados[2];
    int num_mercados;

    // sem_t *mutex_user1;
    // sem_t *mutex_user2;
    // sem_t *mutex_login;

    pid_t childs_pid[2];

} SM;

int shm_id;
SM *shared_memory;

// funcao que le o ficheiro
void config(char *path) {
    FILE *fich = fopen(path, "r");
    assert(fich);

    char line[200];

    fscanf(fich, "%s", line);
    char *token = strtok(line, "/");
    int i = 0;
    while (token != NULL) {
        strcpy(shared_memory->admin[i++], token);
        token = strtok(NULL, "/");
    }
    fscanf(fich, "%s", line);
    // guardar utilizadores
    shared_memory->num_utilizadores = atoi(line);
    int num = 0;
    while (num < shared_memory->num_utilizadores) {
        fscanf(fich, "%s", line);
        char *token = strtok(line, ";");
        i = 0;
        while (token != NULL) {
            if (i == 0) {
                strcpy(shared_memory->users[num].nome, token);
                i++;
            }

            else if (i == 1) {
                strcpy(shared_memory->users[num].password, token);
                i++;
            } else if (i == 2) {
                char *pEnd;
                shared_memory->users[num++].saldo_inicial = strtof(token, &pEnd);
            }
            token = strtok(NULL, ";");
        }
    }

    // guardar os mercados
    while (fscanf(fich, "%[^\n] ", line) != EOF) {
        char *token = strtok(line, ";");
        i = 0;
        while (token != NULL) {
            int merc = 0;
            if (shared_memory->num_mercados != 0) {
                char mercado[50];
                strcpy(mercado, shared_memory->mercados[0].nome);
                if (strcmp(mercado, line)) {
                    merc = 1;
                    if (shared_memory->mercados[merc].num_acoes == 0)
                        shared_memory->mercados[merc].num_acoes++;
                }
            }
            if (shared_memory->num_mercados == 0)
                shared_memory->num_mercados++;
            int a = shared_memory->mercados[merc].num_acoes;
            if (i == 0) {
                strcpy(shared_memory->mercados[merc].nome, token);
                i++;
            } else if (i == 1) {
                strcpy(shared_memory->mercados[merc].acoes[a].nome, token);
                i++;
            } else if (i == 2) {
                shared_memory->mercados[merc].acoes[a].preco_inicial = (float)atof(token);
            }
            token = strtok(NULL, ";");
        }
    }
    fclose(fich);
}

void erro(char *msg) {
    perror(msg);
    exit(1);
}

int login(int fd);

int main(int argc, char **argv) {
    int fd, client;
    struct sockaddr_in addr, client_addr;
    int client_addr_size;

    if (argc != 4) {
        erro("stock_server {PORTO_BOLSA} {PORTO_CONFIG} {ficheiro configuração}");
        exit(-1);
    }

    short SERVER_PORT = (short)atoi(argv[1]);
    short SERVER_CONFIG = (short)atoi(argv[2]);
    char path[200];
    strcpy(path, argv[3]);

    bzero((void *)&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(SERVER_PORT);

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        erro("na funcao socket");
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        erro("na funcao bind");
    if (listen(fd, 11) < 0)
        erro("na funcao listen");

    client_addr_size = sizeof(client_addr);

    // Open shared memory
    shm_id = shmget(IPC_PRIVATE, sizeof(SM), IPC_CREAT | IPC_EXCL | 0700);

    // Attach
    shared_memory = shmat(shm_id, NULL, 0);

    config(path);

    // sem_unlink("MUTEX_USER1");
    // sem_unlink("MUTEX_USER2");
    // sem_unlink("MUTEX_LOGIN");
    // shared_memory->mutex_user1 = sem_open("MUTEX_USER1", O_CREAT | O_EXCL, 0700, 1);
    // shared_memory->mutex_user2 = sem_open("MUTEX_USER2", O_CREAT | O_EXCL, 0700, 0);
    // shared_memory->mutex_login = sem_open("MUTEX_LOGIN", O_CREAT | O_EXCL, 0700, 0);
    // shared_memory->mensagem_invalida = 0;
    printf("%s",shared_memory->users[0].nome);

    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
    //     // wait for new connection
    client = accept(fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_size);
    printf("Client connected...\n");

    login(client);

    // while (jogador < 11) {
    //     // clean finished child processes, avoiding zombies
    //     // must use WNOHANG or would block whenever a child process was working
    //     while (waitpid(-1, NULL, WNOHANG) > 0)
    //         ;
    //     // wait for new connection
    //     client = accept(fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_size);

    //     // Store client socket
    //     shared_memory->users[jogador++]->clients_fd = client;

    //     printf("Client %d connected...\n", jogador);
    //     pid_t cpid = fork();
    //     if (cpid == 0) {
    //         // process_client(jogador);
    //         close(fd);
    //         close(client);
    //         exit(0);
    //     }

    //     // shared_memory->childs_pid[jogador - 1] = cpid;
    // }

    // wait(&status);
    // if (WEXITSTATUS(status) == 1)
    //     kill(shared_memory->childs_pid[1], SIGKILL);
    // else
    //     kill(shared_memory->childs_pid[0], SIGKILL);

    // close(shared_memory->clients_fd[0]);
    // close(shared_memory->clients_fd[1]);

    // terminar();

    // close socket
    close(fd);

    return 0;
}

int login(int fd) {
    char username[30];
    char password[30];

    // Read username
    write(fd, "Login:\n Username: ", 50);
    fflush(stdout);
    read(fd, username, 30);
    // Verify user
    int i;
    int existe = 0;
    for (i = 0; i < shared_memory->num_utilizadores; i++) {
        char aux[50];
        strcpy(aux,shared_memory->users[i].nome);
        if (!strcmp(username, aux)) {
            existe = 1;
            break;
        }
    }

    // Read password
    write(fd, "\nPassword: ", 50);
    read(fd, password, BUF_SIZE);
    // Verify password
    int password_correta = 0;
    if (existe) {
        char aux[50];
        strcpy(aux,shared_memory->users[i].password);
        if (!strcmp(password, aux)) {
            password_correta = 1;
        }
    }

    // Return success or unsuccess
    if (!existe) {
        write(fd, "\nO Username nao existe", 50);
        return 0;
    } else if (existe && !password_correta) {
        write(fd, "\nPassword incorreta", 50);
        return 0;
    } else {
        write(fd, "\nLogin efetuado com sucesso!", 50);
        return 1;
    }
}
