#include "func.h"

void erro(char *msg) {
    perror(msg);
    exit(1);
}

// funcao que le o ficheiro
int config(char *path) {
    FILE *fich = fopen(path, "r");
    assert(fich);

    char line[200];

    // Administrador
    fscanf(fich, "%s", line);
    char *token = strtok(line, "/");
    int i = 0;
    while (token != NULL) {
        strcpy(shared_memory->admin[i++], token);
        token = strtok(NULL, "/");
    }

    // Utilizadores
    fscanf(fich, "%s", line);
    shared_memory->num_utilizadores = atoi(line);
    if (shared_memory->num_utilizadores > 5) {
        printf("Demasiados users inseridos no ficheiro de configuracao (maximo de 5)");
        return -1;
    }

    int num = 0;
    while (num < shared_memory->num_utilizadores) {

        fscanf(fich, "%s", line);
        char linha[BUF_SIZE];
        strcpy(linha, line);
        char *token = strtok(line, ";");

        // Verificar se estao o numero correto de parametros no ficheiro
        int count = 0;
        while (token != NULL) {
            count++;
            token = strtok(NULL, ";");
        }

        if (count < 4 || count > 5) {
            printf("\nNumero de argumentos errados na linha %d do ficheiro de configuracao.\n", num + 3);
            return -1;
        }

        // printf("linha : %s\n", line);

        char *token2 = strtok(linha, ";");
        if (count == 4) { // O user so tem 1 mercado
            i = 0;
            while (token2 != NULL) {
                if (i == 0) {
                    shared_memory->users[num].num_mercados = 1;
                    shared_memory->users[num].num_acoes_compradas = 0;
                    strcpy(shared_memory->users[num].nome, token2);
                } else if (i == 1) {
                    strcpy(shared_memory->users[num].password, token2);
                } else if (i == 2) {
                    strcpy(shared_memory->users[num].mercados[0].nome, token2);
                } else if (i == 3) {
                    char *pEnd;
                    shared_memory->users[num].ocupado = true;
                    shared_memory->users[num].saldo = strtof(token2, &pEnd);
                }
                i++;
                token2 = strtok(NULL, ";");
            }

        } else { // O user tem 2 mercados
            i = 0;
            while (token2 != NULL) {
                if (i == 0) {
                    shared_memory->users[num].num_mercados = 2;
                    strcpy(shared_memory->users[num].nome, token2);
                } else if (i == 1) {
                    strcpy(shared_memory->users[num].password, token2);
                } else if (i == 2) {
                    strcpy(shared_memory->users[num].mercados[0].nome, token2);
                } else if (i == 3) {
                    strcpy(shared_memory->users[num].mercados[1].nome, token2);
                } else if (i == 4) {
                    char *pEnd;
                    shared_memory->users[num].ocupado = true;
                    shared_memory->users[num].saldo = strtof(token2, &pEnd);
                }
                i++;
                token2 = strtok(NULL, ";");
            }
        }
        num++;
    }

    // guardar os mercados
    int merc = 0;
    int condicao = 0;
    while (fscanf(fich, "%s", line) != EOF) {
        char *token = strtok(line, ";");
        int l = 0;
        while (token != NULL) {
            srand(time(NULL));
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
                    shared_memory->mercados[merc].acoes[shared_memory->mercados[merc].num_acoes].preco = (float)atof(token);
                    shared_memory->mercados[merc].num_acoes++;
                    int n_acoes_inicial = rand() % 100 + 10;
                    shared_memory->mercados[merc].acoes[shared_memory->mercados[merc].num_acoes].n_acoes = n_acoes_inicial;
                }
                token = strtok(NULL, ";");
                l++;
            } else {
                printf("Numero de acoes do mercado %s em excesso no ficheiro de configuracao!\n", shared_memory->mercados[merc].nome);
                return -1;
            }
        }
    }

    // colocar acoes em user novos
    if (shared_memory->num_mercados == 0) {
        printf("Nao existem mercados registados!\n");
        exit(0);
    }
    for (int i = 0; i < shared_memory->num_utilizadores; i++) {
        for (int m = 0; m < shared_memory->users[i].num_mercados; m++) {
            if (!strcmp(shared_memory->users[i].mercados[m].nome, shared_memory->mercados[0].nome)) {
                for (int a = 0; a < shared_memory->mercados[0].num_acoes; a++) {
                    shared_memory->users[i].mercados[m].acao[a].n_acoes = 0;
                    strcpy(shared_memory->users[i].mercados[m].acao[a].nome, shared_memory->mercados[0].acoes[a].nome);
                }
                shared_memory->users[i].mercados[m].num_acoes = shared_memory->mercados[0].num_acoes;
                shared_memory->users[i].mercados[m].n_acoes_comp_mercado = 0;
            } else if (!strcmp(shared_memory->users[i].mercados[m].nome, shared_memory->mercados[1].nome)) {
                for (int a = 0; a < shared_memory->mercados[1].num_acoes; a++) {
                    shared_memory->users[i].mercados[m].acao[a].n_acoes = 0;
                    strcpy(shared_memory->users[i].mercados[m].acao[a].nome, shared_memory->mercados[1].acoes[a].nome);
                }
                shared_memory->users[i].mercados[m].num_acoes = shared_memory->mercados[0].num_acoes;
                shared_memory->users[i].mercados[m].n_acoes_comp_mercado = 0;
            } else {
                printf("Verifique os mercados inseridos no user %s!\n", shared_memory->users[i].nome);
                exit(0);
            }
        }
    }

    fclose(fich);

    return 0;
}

int login(int fd, char *username) {
    char buffer[BUF_SIZE];

    if (shared_memory->num_utilizadores == 0) {
        printf("Nao existem usuarios registados!");
        exit(1);
    }

    // Read username
    memset(buffer, 0, BUF_SIZE);
    snprintf(buffer, BUF_SIZE, "Login\nUsername: ");
    write(fd, buffer, BUF_SIZE); //
    printf("1\n");
    // fflush(stdout);
    memset(buffer, 0, BUF_SIZE);
    read(fd, buffer, BUF_SIZE); //
    // strcpy(username, buffer);

    // Verify user
    // buffer[strlen(buffer) - 1] = '\0'; // Netcat
    int a;
    int i;
    int existe = 0;
    for (i = 0; i < 10; i++) {
        char aux[BUF_SIZE];
        if (shared_memory->users[i].ocupado == true) {
            strcpy(aux, shared_memory->users[i].nome);
            if ((a = strcmp(buffer, aux)) == 0) {
                existe = 1;
                break;
            }
        }
    }

    // Read password
    memset(buffer, 0, BUF_SIZE);
    snprintf(buffer, BUF_SIZE, "Password: ");
    write(fd, buffer, BUF_SIZE); //
    printf("2\n");
    memset(buffer, 0, BUF_SIZE);
    read(fd, buffer, BUF_SIZE); //
    // buffer[strlen(buffer) - 1] = '\0'; // Netcat

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
        printf("3\n");
        return -1;
    } else if (existe && !password_correta) {
        snprintf(buffer, BUF_SIZE, "\nPassword incorreta");
        write(fd, buffer, BUF_SIZE);
        printf("4\n");
        return -1;
    } else {
        snprintf(buffer, BUF_SIZE, "\nLogin efetuado com sucesso!");
        write(fd, buffer, BUF_SIZE);
        printf("5\n");
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

void process_client(int client_fd, int id) {
    char buffer[BUF_SIZE];

    while (1) {
        // Verificar a escolha do cliente
        printf("while\n");
        memset(buffer, 0, BUF_SIZE);
        read(client_fd, buffer, BUF_SIZE);

        if (!strcmp(buffer, "escolha1")) {

        } else if (!strcmp(buffer, "escolha2")) {
            // variaveis
            char carteira[BUF_SIZE * 2];
            // listar os mercados e acoes que tem acesso!
            snprintf(carteira, BUF_SIZE * 2, "Informacoes dos mercados a que tem acesso:\n");
            if (shared_memory->users[id].num_mercados == 0) {
                snprintf(carteira, BUF_SIZE, "Voce nao tem acoes nao tem acesso a nenhum mercado!");
                strcat(carteira, "\n");
                write(client_fd, carteira, BUF_SIZE * 2);
                printf("6\n");
            } else {
                snprintf(carteira, BUF_SIZE * 2, "Informacoes dos mercados a que tem acesso:\n");
                for (int i = 0; i < shared_memory->users[id].num_mercados; i++) {
                    memset(buffer, 0, BUF_SIZE);
                    snprintf(buffer, BUF_SIZE, "Mercado : %s\n", shared_memory->users[id].mercados[i].nome);
                    strcat(carteira, buffer);
                    for (int j = 0; j < shared_memory->users[id].mercados[i].num_acoes; j++) {
                        if (!strcmp(shared_memory->users[id].mercados[0].nome, shared_memory->mercados[0].nome)) {
                            memset(buffer, 0, BUF_SIZE);
                            snprintf(buffer, BUF_SIZE, "   Nome da acao: %s; Preco: %f\n", shared_memory->users[id].mercados[i].acao[j].nome, shared_memory->mercados[0].acoes[j].preco);
                            strcat(carteira, buffer);
                        } else if (!strcmp(shared_memory->users[id].mercados[i].nome, shared_memory->mercados[1].nome)) {
                            memset(buffer, 0, BUF_SIZE);
                            snprintf(buffer, BUF_SIZE, "   Nome da acao: %s; Preco: %f\n", shared_memory->users[id].mercados[i].acao[j].nome, shared_memory->mercados[1].acoes[j].preco);
                            strcat(carteira, buffer);
                        }
                    }
                }
                strcat(carteira, "\n");
                write(client_fd, carteira, BUF_SIZE * 2);
                printf("7\n");

                // fazer compra!!
                // TODO:
            }

        } else if (!strcmp(buffer, "escolha3")) {

        } else if (!strcmp(buffer, "escolha4")) {

        } else if (!strcmp(buffer, "escolha5")) {
            printf("ENTREI!!\n");
            char carteira[BUF_SIZE * 2];
            snprintf(carteira, BUF_SIZE * 2, "Informacoes da carteira:\nSaldo disponivel: %f", shared_memory->users[id].saldo);
            if (shared_memory->users[id].num_acoes_compradas == 0) {
                strcat(carteira, "/ Voce nao tem acoes na carteira!");
            } else {
                for (int i = 0; i < shared_memory->users[id].num_mercados; i++) {
                    int count = 0;
                    memset(buffer, 0, BUF_SIZE);
                    snprintf(buffer, BUF_SIZE, "Mercado : %s\n", shared_memory->users[id].mercados[i].nome);
                    strcat(carteira, buffer);
                    for (int j = 0; j < shared_memory->users[id].mercados[i].num_acoes; j++) {
                        if (shared_memory->users[id].mercados[i].acao[j].n_acoes > 0) {
                            memset(buffer, 0, BUF_SIZE);
                            snprintf(buffer, BUF_SIZE, "   Nome da acao: %s; Quantidade: %d\n", shared_memory->users[id].mercados[i].acao[j].nome, shared_memory->users[id].mercados[i].acao[j].n_acoes);
                            strcat(carteira, buffer);
                            count++;
                        }
                    }
                    memset(buffer, 0, BUF_SIZE);
                    snprintf(buffer, BUF_SIZE, "   Voce nao tem acoes na sua carteira deste mercado\n");
                    strcat(carteira, buffer);
                }
            }
            strcat(carteira, "\n");
            printf("%s", carteira);
            write(client_fd, carteira, BUF_SIZE * 2);
            printf("8\n");

        } else if (!strcmp(buffer, "escolha6")) {
            break;
        }
    }
}

void terminar(int shm_id) {

    sem_close(shared_memory->sem_compras);
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
