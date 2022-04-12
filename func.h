#ifndef FUNC_H
#define FUNC_H

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
#include<errno.h>

#define BUF_SIZE 1024

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
    char nome[50];
    char password[50];
    float saldo_inicial;
    mercado mercados[2];
} user;

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

void erro(char *msg);

int login(int fd, SM *shared_memory);

int login_admin(int s, SM *shared_memory);

void config(char *path, SM *shared_memory);

#endif