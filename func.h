#ifndef FUNC_H
#define FUNC_H

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define BUF_SIZE 1024
#define GROUP1 "239.0.0.1"
#define GROUP2 "239.0.0.2"
#define PORTO1 9876
#define PORTO2 9875

int fd;
int s;

typedef struct {
    char nome[500];
    float preco;
    int n_acoes;
} acao;

typedef struct {
    char nome[500];
    acao acoes[3];
    int num_acoes;
} mercado;

typedef struct {
    char nome[500];
    int n_acoes;
} acoes;

typedef struct {
    char nome[500];
    acoes acao[3];
    int num_acoes;
    int n_acoes_comp_mercado;
    bool ocupado;
    bool acesso;
} mercadosUser;

typedef struct {
    char nome[500];
    char password[500];
    float saldo;
    mercadosUser mercados[2];
    int num_mercados;
    int num_acoes_compradas;
    bool ocupado;
} utilizadores;

typedef struct {
    pid_t c_pid;
    bool ocupado;
} processo;

//------

typedef struct {
    char admin[2][500]; // [0]AdminName [1]AdminPassword
    int num_utilizadores;
    utilizadores users[10];
    mercado mercados[2];
    int num_mercados;
    int refresh_time;

    int clientes_atuais; // numero de clientes a acessar o servidor ao mesmo tempo
    int refresh_pid;

    sem_t *sem_compras;
    sem_t *sem_users;
    // sem_t *mutex_login;
    processo atuais[5];

} SM;

int shm_id;
SM *shared_memory;

void erro(char *msg);

int login(int fd, char *username);

int login_admin(int s);

int process_client(int client_fd);

int config(char *path);

void terminar(int shm_id);

void add_cpid(int cliente);

void remove_cpid(int cliente);

void SIGINT_HANDLER(int signum);

#endif