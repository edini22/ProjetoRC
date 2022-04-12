#include "func.h"

void erro(char *msg) {
    perror(msg);
    exit(1);
}

int login(int fd, SM *shared_memory) {
    char buffer[BUF_SIZE];
    memset(buffer, 0, BUF_SIZE);

    // Read username
    snprintf(buffer, BUF_SIZE, "Login:\nUsername: ");
    write(fd, buffer, BUF_SIZE);
    fflush(stdout);
    memset(buffer, 0, BUF_SIZE);
    read(fd, buffer, BUF_SIZE);

    // Verify user
    buffer[strlen(buffer) - 1] = '\0'; // FIXME: tirar esta linha quando deixarmos de usar o nc
    int a;
    int i;
    int existe = 0;
    for (i = 0; i < shared_memory->num_utilizadores; i++) {
        char aux[BUF_SIZE];
        strcpy(aux, shared_memory->users[i].nome);
        if ((a = strcmp(buffer, aux)) == 0) {
            existe = 1;
            break;
        }
    }

    // Read password
    memset(buffer, 0, BUF_SIZE);
    snprintf(buffer, BUF_SIZE, "Password: ");
    write(fd, buffer, BUF_SIZE);
    memset(buffer, 0, BUF_SIZE);
    read(fd, buffer, BUF_SIZE);
    buffer[strlen(buffer) - 1] = '\0'; // FIXME: tirar esta linha quando deixarmos de usar o nc

    // Verify password
    int password_correta = 0;
    if (existe) {
        char aux[50];
        strcpy(aux, shared_memory->users[i].password);
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

int login_admin(int s, SM *shared_memory) {
    struct sockaddr_in admin_outra;
    char buffer[BUF_SIZE];
    memset(buffer, 0, BUF_SIZE);
    socklen_t slen = sizeof(admin_outra);
    int recv_len, send_len;

    // Read username
    snprintf(buffer, BUF_SIZE, "Login:\nUsername(Admin): ");
    if (sendto(s, buffer, strlen(buffer), 0, (struct sockaddr *)&admin_outra, slen) == -1)
        erro("funcao sendto\n");
    fflush(stdout);
    memset(buffer, 0, BUF_SIZE);
    if (recv_len = recvfrom(s, buffer, BUF_SIZE, 0, (struct sockaddr *)&admin_outra, (socklen_t *)&slen) == -1)
        erro("funcao recvfrom");
    printf("Teste\n");

    // Verify user
    buffer[recv_len - 1] = '\0'; // FIXME: tirar esta linha quando deixarmos de usar o nc
    int i;
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
    buffer[recv_len - 1] = '\0'; // FIXME: tirar esta linha quando deixarmos de usar o nc

    // Verify password
    int password_correta = 0;
    if (existe) {
        char aux[50];
        strcpy(aux, shared_memory->admin[1]);
        if (!strcmp(buffer, aux) == 0) {
            password_correta = 1;
        }
    }

    // Return success or unsuccess
    memset(buffer, 0, BUF_SIZE);
    if (!existe) {
        snprintf(buffer, BUF_SIZE, "\nO Username nao existe");
        sendto(s, buffer, strlen(buffer), 0, (struct sockaddr *)&admin_outra, slen);
        return -1;
    } else if (existe && !password_correta) {
        snprintf(buffer, BUF_SIZE, "\nPassword incorreta");
        sendto(s, buffer, strlen(buffer), 0, (struct sockaddr *)&admin_outra, slen);
        return -1;
    } else {
        snprintf(buffer, BUF_SIZE, "\nLogin efetuado com sucesso!");
        sendto(s, buffer, strlen(buffer), 0, (struct sockaddr *)&admin_outra, slen);
        return 1;
    }
}

// funcao que le o ficheiro
void config(char *path, SM *shared_memory) {
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
