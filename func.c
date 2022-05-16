#include "func.h"

void erro(char *msg) {
    perror(msg);
    exit(1);
}

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
    if (shared_memory->num_utilizadores > 5) {
        printf("Demasiados users inseridos no ficheiro de configuracao, maximo de 5");
        exit(1);
    }
    int num = 0;
    while (num < shared_memory->num_utilizadores) {
        fscanf(fich, "%s", line);
        char *token = strtok(line, ";");
        i = 0;
        while (token != NULL) {
            if (i == 0) {
                shared_memory->users[num].user.num_mercados = 0;
                strcpy(shared_memory->users[num].user.nome, token);
                i++;
            }

            else if (i == 1) {
                strcpy(shared_memory->users[num].user.password, token);
                i++;
            } else if (i == 2) {
                char *pEnd;
                shared_memory->users[num].ocupado = true;
                shared_memory->users[num++].user.saldo_inicial = strtof(token, &pEnd);
            }
            token = strtok(NULL, ";");
        }
    }

    // guardar os mercados
    int merc = 0;
    int condicao = 0;
    while (fscanf(fich, "%s", line) != EOF) {
        char *token = strtok(line, ";");
        int l = 0;
        while (token != NULL) {
            if (condicao != 0 && l == 0) {
                char mercado[BUF_SIZE];
                strcpy(mercado, shared_memory->mercados[0].nome);
                if (strcmp(mercado, token))
                    merc = 1;
                else
                    merc = 0;
            }
            condicao++;
            if (shared_memory->mercados[merc].num_acoes < 3) {
                if (l == 0 && shared_memory->mercados[merc].num_acoes == 0) {
                    strcpy(shared_memory->mercados[merc].nome, line);
                    shared_memory->num_mercados++;
                } else if (l == 1) {
                    strcpy(shared_memory->mercados[merc].acoes[shared_memory->mercados[merc].num_acoes].nome, token);
                } else if (l == 2) {
                    shared_memory->mercados[merc].acoes[shared_memory->mercados[merc].num_acoes].preco_inicial = (float)atof(token);
                    shared_memory->mercados[merc].num_acoes++;
                }
                token = strtok(NULL, ";");
                l++;
            } else {
                printf("Numero de acoes do mercado %s em excesso!\n", shared_memory->mercados[merc].nome);
                exit(1);
            }
        }
    }
    fclose(fich);
}

int login(int fd) {
    char buffer[BUF_SIZE];
    memset(buffer, 0, BUF_SIZE);

    if (shared_memory->num_utilizadores == 0) {
        printf("Nao existem usuarios registados!");
        exit(1);
    }

    // Read username
    snprintf(buffer, BUF_SIZE, "Login\nUsername: ");
    write(fd, buffer, BUF_SIZE);
    fflush(stdout);
    memset(buffer, 0, BUF_SIZE);
    read(fd, buffer, BUF_SIZE);

    // Verify user
    // buffer[strlen(buffer) - 1] = '\0'; // Netcat
    int a;
    int i;
    int existe = 0;
    for (i = 0; i < 10; i++) {
        char aux[BUF_SIZE];
        if (shared_memory->users[i].ocupado == true) {
            strcpy(aux, shared_memory->users[i].user.nome);
            if ((a = strcmp(buffer, aux)) == 0) {
                existe = 1;
                break;
            }
        }
    }

    // Read password
    memset(buffer, 0, BUF_SIZE);
    snprintf(buffer, BUF_SIZE, "Password: ");
    write(fd, buffer, BUF_SIZE);
    memset(buffer, 0, BUF_SIZE);
    read(fd, buffer, BUF_SIZE);
    // buffer[strlen(buffer) - 1] = '\0'; // Netcat

    // Verify password
    int password_correta = 0;
    if (existe) {
        char aux[50];
        strcpy(aux, shared_memory->users[i].user.password);
        if ((a = strcmp(buffer, aux)) == 0) {
            password_correta = 1;
        }
    }

    // Return success or unsuccess
    memset(buffer, 0, BUF_SIZE);
    if (!existe) {
        snprintf(buffer, BUF_SIZE, "\nO Username nao existe");
        write(fd, buffer, BUF_SIZE);
        return -1;
    } else if (existe && !password_correta) {
        snprintf(buffer, BUF_SIZE, "\nPassword incorreta");
        write(fd, buffer, BUF_SIZE);
        return -1;
    } else {
        snprintf(buffer, BUF_SIZE, "\nLogin efetuado com sucesso!");
        write(fd, buffer, BUF_SIZE);
        return i;
    }
}

int login_admin(int s) {
    struct sockaddr_in admin_outra;
    char buffer[BUF_SIZE];
    memset(buffer, 0, BUF_SIZE);
    socklen_t slen = sizeof(admin_outra);
    int recv_len, send_len;

    // O admin comeca por enviar o username
    memset(buffer, 0, BUF_SIZE);
    if (recv_len = recvfrom(s, buffer, BUF_SIZE, 0, (struct sockaddr *)&admin_outra, (socklen_t *)&slen) == -1)
        erro("funcao recvfrom");

    // Verify user
    // buffer[strlen(buffer) + 1] = '\0'; // Netcat
    int i;
    int a;
    int existe = 0;
    char aux[BUF_SIZE];
    strcpy(aux, shared_memory->admin[0]);
    if (!strcmp(buffer, aux)) {
        existe = 1;
    }

    // Read password
    memset(buffer, 0, BUF_SIZE);
    snprintf(buffer, BUF_SIZE, "Password: ");
    sendto(s, buffer, strlen(buffer), 0, (struct sockaddr *)&admin_outra, slen);
    memset(buffer, 0, BUF_SIZE);
    recv_len = recvfrom(s, buffer, BUF_SIZE, 0, (struct sockaddr *)&admin_outra, (socklen_t *)&slen);
    // buffer[strlen(buffer) + 1] = '\0'; // Netcat

    // Verify password
    int password_correta = 0;
    if (existe) {
        char aux[50];
        strcpy(aux, shared_memory->admin[1]);
        if (!strcmp(buffer, aux)) {
            password_correta = 1;
        }
    }

    // Return success or unsuccess
    memset(buffer, 0, BUF_SIZE);
    if (!existe) {
        snprintf(buffer, BUF_SIZE, "\nO Username (Admin) nao existe\n");
        sendto(s, buffer, strlen(buffer), 0, (struct sockaddr *)&admin_outra, slen);
        return -1;
    } else if (existe && !password_correta) {
        snprintf(buffer, BUF_SIZE, "\nPassword incorreta\n");
        sendto(s, buffer, strlen(buffer), 0, (struct sockaddr *)&admin_outra, slen);
        return -1;
    } else {
        snprintf(buffer, BUF_SIZE, "\nLogin (admin) efetuado com sucesso!\nClique enter para continuar");
        sendto(s, buffer, strlen(buffer), 0, (struct sockaddr *)&admin_outra, slen);
        printf("Login (admin) efetuado com sucesso!\n");
        return 1;
    }
}

void terminar(int shm_id) {

    sem_close(shared_memory->mutex_compras);
    // sem_close(shared_memory->mutex_menu);
    sem_unlink("MUTEX_COMPRAS");
    // sem_unlink("MUTEX_MENU");
    shmdt(shared_memory);
    shmctl(shm_id, IPC_RMID, NULL);
}

void add_cpid(int cliente) {
    for (int i = 0; i < 5; i++) {
        if (shared_memory->atuais[i].ocupado == false) {
            shared_memory->atuais[i].ocupado == true;
            shared_memory->atuais[i].c_pid = cliente;
            shared_memory->clientes_atuais++;
            break;
        }
    }
}

void remove_cpid(int cliente) {
    for (int i = 0; i < 5; i++) {
        if (cliente == shared_memory->atuais[i].ocupado) {
            shared_memory->atuais[i].ocupado = false;
            shared_memory->clientes_atuais--;
            break;
        }
    }
}
