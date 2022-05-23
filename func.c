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
                    shared_memory->users[num].mercados[1].ocupado = false;
                    shared_memory->users[num].mercados[1].acesso = false;
                    shared_memory->users[num].num_mercados = 1;
                    shared_memory->users[num].num_acoes_compradas = 0;
                    strcpy(shared_memory->users[num].nome, token2);
                } else if (i == 1) {
                    strcpy(shared_memory->users[num].password, token2);
                } else if (i == 2) {
                    strcpy(shared_memory->users[num].mercados[0].nome, token2);
                    shared_memory->users[num].mercados[0].ocupado = true;
                    shared_memory->users[num].mercados[0].acesso = true;
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
                    shared_memory->users[num].mercados[0].ocupado = true;
                    shared_memory->users[num].mercados[0].acesso = true;
                } else if (i == 3) {
                    strcpy(shared_memory->users[num].mercados[1].nome, token2);
                    shared_memory->users[num].mercados[1].ocupado = true;
                    shared_memory->users[num].mercados[1].acesso = true;
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
    buffer[strlen(buffer) - 1] = '\0'; // Netcat
    printf("%s | %s\n", buffer, shared_memory->admin[0]);
    // printf("\t%s",shared_memory->admin[0]);

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
    buffer[strlen(buffer) - 1] = '\0'; // Netcat

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

int process_client(int client_fd) {
    char buffer[BUF_SIZE];
    int id = login(client_fd, buffer);
    if (id == -1) {
        printf("Wrong Username or password\n");
    } else {
        add_cpid(client_fd);

        printf("Cliente n%d logado (%s)\n", shared_memory->clientes_atuais, shared_memory->users[id].nome);

        // Enviar ao cliente os mercados que pode aceder
        memset(buffer, 0, BUF_SIZE);
        sem_wait(shared_memory->sem_users);
        int n_merc = shared_memory->users[id].num_mercados;
        if (n_merc != 0) {
            char aux[200];
            snprintf(buffer, BUF_SIZE, "Mercados disponiveis:\n");
            printf("%s", buffer);
            for (int i = 0; i < n_merc; i++) {
                memset(aux, 0, 200);
                printf("\t%s / n_acoes: %d\n", shared_memory->users[id].mercados[i].nome, shared_memory->users[id].mercados[i].num_acoes);
                snprintf(aux, BUF_SIZE, "\t%s\n", shared_memory->users[id].mercados[i].nome);
                strcat(buffer, aux);
            }
        } else {
            snprintf(buffer, BUF_SIZE, "Nao tem acesso a nenhum mercado!!\n");
        }
        sem_post(shared_memory->sem_users);
        write(client_fd, buffer, BUF_SIZE);

        while (1) {
            // Verificar a escolha do cliente
            // printf("while\n");
            memset(buffer, 0, BUF_SIZE);
            read(client_fd, buffer, BUF_SIZE);

            if (!strcmp(buffer, "escolha1")) {
                sem_wait(shared_memory->sem_users);
                // Mostrar ao user os mercados disponiveis
                int n_merc = shared_memory->users[id].num_mercados;
                if (n_merc != 0) {
                    char aux[200];
                    snprintf(buffer, BUF_SIZE, "Escolha um mercado para subscrever:\n");
                    printf("%s", buffer);
                    for (int i = 0; i < n_merc; i++) {
                        memset(aux, 0, 200);
                        snprintf(aux, BUF_SIZE, "%d - %s\n", i + 1, shared_memory->users[id].mercados[i].nome);
                        printf("%s", aux);
                        strcat(buffer, aux);
                    }
                } else {
                    snprintf(buffer, BUF_SIZE, "Nao tem acesso a nenhum mercado pelo que nao pode subscrever a nenhuma cotacao\n");
                    printf("%s", buffer);
                }
                sem_post(shared_memory->sem_users);
                write(client_fd, buffer, BUF_SIZE);

                // Receber o numero do mercado a subscrever
                char numero[2];
                printf("Numero de acoes = %d\n", n_merc);
                read(client_fd, numero, 2);
                int num = atoi(numero);
                memset(buffer, 0, BUF_SIZE);
                if (num > n_merc || num < 1) {
                    snprintf(buffer, BUF_SIZE, "O numero nao e valido\n");
                } else {
                    num--; // para o indice ficar correto
                    int mercado;
                    for (int i = 0; i < shared_memory->num_mercados; i++) {
                        for (int j = 0; j < shared_memory->users[id].num_mercados; j++) {
                            // Encontrar o indice do mercado no array dos mercados
                            if (!strcmp(shared_memory->mercados[i].nome, shared_memory->users[id].mercados[j].nome)) {
                                // TODO: Devolver endereco do multicast
                                // QUESTION: nao sei se temos de fazer mais alguma coisa a nao ser mandar o endereco...
                                snprintf(buffer, BUF_SIZE, "Mercado escolhido: %s\n", shared_memory->users[id].mercados[j].nome);
                                mercado = j;
                            }
                        }
                    }

                    // QUESTION: ...ou se vamos precisar disto
                    // Multicast para o mercado bvl
                    if (mercado == 0){
                    
                    // Multicast para o mercado nyse
                    } else if (mercado ==1){

                    }

                }

                write(client_fd, buffer, BUF_SIZE);

            } else if (!strcmp(buffer, "escolha2")) {
                // variaveis
                char carteira[BUF_SIZE * 2];
                // listar os mercados e acoes que tem acesso!
                int value;
                sem_getvalue(shared_memory->sem_compras, &value);
                printf("compras-> %d, ", value);
                sem_getvalue(shared_memory->sem_users, &value);
                printf("users -> %d \n", value);
                sem_wait(shared_memory->sem_users);
                if (shared_memory->users[id].num_mercados == 0) {
                    sem_post(shared_memory->sem_users);
                    snprintf(carteira, BUF_SIZE, "Voce nao tem acoes nao tem acesso a nenhum mercado!");
                } else {
                    snprintf(carteira, BUF_SIZE * 2, "Informacoes dos mercados a que tem acesso:\n");
                    for (int i = 0; i < 2; i++) {
                        sem_wait(shared_memory->sem_compras);
                        if (shared_memory->users[id].mercados[i].acesso == true) {
                            memset(buffer, 0, BUF_SIZE);
                            snprintf(buffer, BUF_SIZE, "Mercado : %s\n", shared_memory->users[id].mercados[i].nome);
                            strcat(carteira, buffer);
                            for (int j = 0; j < shared_memory->users[id].mercados[i].num_acoes; j++) {
                                if (!strcmp(shared_memory->users[id].mercados[0].nome, shared_memory->mercados[0].nome)) {
                                    memset(buffer, 0, BUF_SIZE);
                                    snprintf(buffer, BUF_SIZE, "   Nome da acao: %s; Preco: %.3f\n", shared_memory->users[id].mercados[i].acao[j].nome, (shared_memory->mercados[0].acoes[j].preco + 0.02));
                                    strcat(carteira, buffer);
                                } else if (!strcmp(shared_memory->users[id].mercados[i].nome, shared_memory->mercados[1].nome)) {
                                    memset(buffer, 0, BUF_SIZE);
                                    snprintf(buffer, BUF_SIZE, "   Nome da acao: %s; Preco: %.3f\n", shared_memory->users[id].mercados[i].acao[j].nome, (shared_memory->mercados[1].acoes[j].preco + 0.02));
                                    strcat(carteira, buffer);
                                }
                            }
                        }
                        sem_post(shared_memory->sem_compras);
                    }
                    strcat(carteira, "\n");
                    write(client_fd, carteira, BUF_SIZE * 2);
                    printf("7\n");

                    // COMPRA
                    memset(buffer, 0, BUF_SIZE);
                    read(client_fd, buffer, BUF_SIZE); // mercado/acao/n_acao
                    char *token = strtok(buffer, "/");
                    int count = 0;
                    int continua = 1;
                    int merc, ac;
                    char msg[BUF_SIZE];

                    char buffer2[BUF_SIZE];
                    strcpy(buffer2, buffer);
                    int counti = 0;
                    char *token2 = strtok(buffer2, "/");
                    while (token2 != NULL) {
                        // printf("%s\n", token);
                        counti++;
                        token2 = strtok(NULL, "/");
                    }
                    printf("count -> %d\n", counti);
                    if (counti != 3) {
                        memset(buffer, 0, BUF_SIZE);
                        snprintf(buffer, BUF_SIZE, "Numero de parametros errado!\n");
                        write(client_fd, buffer, BUF_SIZE);
                    } else {
                        while (token != NULL && continua) {
                            int existe = 0;
                            // printf("%s\n", token);
                            if (count == 0) { // MERCADO
                                for (merc = 0; merc < 2 && continua; merc++) {
                                    // Verificar se o user tem acesso
                                    if (shared_memory->users[id].mercados[merc].acesso == true) {
                                        // Encontar o indice do mercado
                                        if (!strcmp(shared_memory->users[id].mercados[merc].nome, token)) {
                                            existe = 1;
                                            break;
                                        }
                                    } else {
                                        snprintf(msg, BUF_SIZE, "Voce nao tem acesso ao mercado que escolheu");
                                        write(client_fd, msg, BUF_SIZE);
                                        continua = 0;
                                    }
                                }
                                if (!existe) {
                                    snprintf(msg, BUF_SIZE, "O mercado que escolheu nao existe");
                                    write(client_fd, msg, BUF_SIZE);
                                    continua = 0;
                                }
                                count++;

                            } else if (count == 1) { // ACAO
                                for (ac = 0; ac < shared_memory->users[id].mercados[merc].num_acoes && continua; ac++) {
                                    // Encontrar o indice da acao
                                    if (!strcmp(shared_memory->users[id].mercados[merc].acao[ac].nome, token)) {
                                        existe = 1;
                                        break;
                                    }
                                }
                                if (!existe) {
                                    snprintf(msg, BUF_SIZE, "A acao que escolheu nao existe");
                                    write(client_fd, msg, BUF_SIZE);
                                    continua = 0;
                                }
                                count++;

                            } else { // QUANTIDADE
                                int n = atoi(token);
                                int total = 0;

                                // Encontrar o indice do mercado e da acao no array dos mercados
                                sem_wait(shared_memory->sem_compras);
                                printf("passei wait_compras\n");
                                for (int m = 0; m < shared_memory->users[id].num_mercados && continua; m++) {
                                    if (!strcmp(shared_memory->users[id].mercados[m].nome, shared_memory->mercados[0].nome)) {
                                        for (int a = 0; a < shared_memory->mercados[0].num_acoes && continua; a++) {
                                            // Verificar se e possivel comprar a quantidade desejada de stocks
                                            if (shared_memory->mercados[0].acoes[a].n_acoes < n) {
                                                sem_post(shared_memory->sem_compras);
                                                snprintf(buffer, BUF_SIZE, "Nao existe essa quantidade de acoes em stock.\n");
                                                write(client_fd, buffer, BUF_SIZE);
                                                continua = 0;
                                                // Encontar o preco do mercado e calcular o total
                                            } else {
                                                sem_post(shared_memory->sem_compras);
                                                total = (shared_memory->mercados[0].acoes[a].preco + 0.02) * n;
                                                continua = 0;
                                                // Verificar se o saldo chega para a compra
                                                if (shared_memory->users[id].saldo < total) {
                                                    snprintf(buffer, BUF_SIZE, "Voce nao tem saldo suficiente.\n");
                                                    write(client_fd, buffer, BUF_SIZE);
                                                    // Realizar a compra efetivamente
                                                } else {
                                                    shared_memory->users[id].saldo -= total;
                                                    shared_memory->users[id].mercados[merc].acao[a].n_acoes += n;
                                                    snprintf(buffer, BUF_SIZE, "Compra efetuada com sucesso.\n");
                                                    write(client_fd, buffer, BUF_SIZE);
                                                }
                                            }
                                        }

                                    } else if (!strcmp(shared_memory->users[id].mercados[m].nome, shared_memory->mercados[1].nome)) {
                                        for (int a = 0; a < shared_memory->mercados[1].num_acoes && continua; a++) {
                                            if (!strcpy(shared_memory->users[id].mercados[merc].acao[ac].nome, shared_memory->mercados[1].acoes[a].nome)) {
                                                // Verificar se e possivel comprar essa quantidade
                                                if (shared_memory->mercados[1].acoes[a].n_acoes < n) {
                                                    sem_post(shared_memory->sem_compras);
                                                    snprintf(buffer, BUF_SIZE, "Nao existe essa quantidade de acoes em stock.\n");
                                                    write(client_fd, buffer, BUF_SIZE);
                                                    continua = 0;
                                                } else {
                                                    sem_post(shared_memory->sem_compras);
                                                    // Encontar o preco do mercado e calcular o total
                                                    total = (shared_memory->mercados[1].acoes[a].preco + 0.02) * n;
                                                    continua = 0;
                                                    // Verificar se o saldo chega para a compra
                                                    if (shared_memory->users[id].saldo < total) {
                                                        snprintf(buffer, BUF_SIZE, "Voce nao tem saldo suficiente.\n");
                                                        write(client_fd, buffer, BUF_SIZE);
                                                        // Realizar a compra efetivamente
                                                    } else {
                                                        shared_memory->users[id].saldo -= total;
                                                        shared_memory->users[id].mercados[merc].acao[a].n_acoes += n;
                                                        snprintf(buffer, BUF_SIZE, "Compra efetuada com sucesso.\n");
                                                        write(client_fd, buffer, BUF_SIZE);
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            token = strtok(NULL, "/");
                        }
                    }

                    sem_post(shared_memory->sem_users);
                }

            } else if (!strcmp(buffer, "escolha3")) {
                // variaveis
                char carteira[BUF_SIZE * 2];
                // Mostrar acoes que comprou previamente
                if (shared_memory->users[id].num_mercados == 0) {
                    snprintf(carteira, BUF_SIZE, "Voce nao tem acoes nao tem acesso a nenhum mercado!\n");
                    strcat(carteira, "\n");
                    write(client_fd, carteira, BUF_SIZE * 2);
                    break;
                } else {
                    snprintf(carteira, BUF_SIZE * 2, "Acoes que possui para venda:\n");
                    for (int i = 0; i < 2; i++) {
                        sem_wait(shared_memory->sem_compras);
                        if (shared_memory->users[id].mercados[i].acesso == true) {
                            memset(buffer, 0, BUF_SIZE);
                            snprintf(buffer, BUF_SIZE, "Mercado : %s\n", shared_memory->users[id].mercados[i].nome);
                            strcat(carteira, buffer);
                            for (int j = 0; j < shared_memory->users[id].mercados[i].num_acoes; j++) {
                                if (!strcmp(shared_memory->users[id].mercados[0].nome, shared_memory->mercados[0].nome)) {
                                    memset(buffer, 0, BUF_SIZE);
                                    snprintf(buffer, BUF_SIZE, "   Nome da acao: %s; Quantidade: %d ;Preco: %.3f\n", shared_memory->users[id].mercados[i].acao[j].nome, shared_memory->users[id].mercados[i].acao[j].n_acoes, shared_memory->mercados[0].acoes[j].preco);
                                    strcat(carteira, buffer);
                                } else if (!strcmp(shared_memory->users[id].mercados[i].nome, shared_memory->mercados[1].nome)) {
                                    memset(buffer, 0, BUF_SIZE);
                                    snprintf(buffer, BUF_SIZE, "   Nome da acao: %s; Quantidade: %d ; Preco: %.3f\n", shared_memory->users[id].mercados[i].acao[j].nome, shared_memory->users[id].mercados[i].acao[j].n_acoes, shared_memory->mercados[1].acoes[j].preco);
                                    strcat(carteira, buffer);
                                }
                            }
                        }
                        sem_post(shared_memory->sem_compras);
                    }
                    strcat(carteira, "\n");
                    write(client_fd, carteira, BUF_SIZE * 2);
                }
                // TODO: vendas :)

            } else if (!strcmp(buffer, "escolha4")) {

            } else if (!strcmp(buffer, "escolha5")) {
                printf("ENTREI!!\n");
                char carteira[BUF_SIZE * 2];
                snprintf(carteira, BUF_SIZE * 2, "Informacoes da carteira:\nSaldo disponivel: %.3f\n", shared_memory->users[id].saldo);
                if (shared_memory->users[id].num_acoes_compradas == 0) {
                    strcat(carteira, "Voce nao tem acoes na carteira!");
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
    return id;
}

void terminar(int shm_id) {

    sem_close(shared_memory->sem_compras);
    sem_close(shared_memory->sem_users);
    sem_unlink("SEM_COMPRAS");
    sem_unlink("SEM_USERS");
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

// Funcao que trata do CTRL-C (termina o programa)
void SIGINT_HANDLER(int signum) {
    sem_close(shared_memory->sem_compras);
    sem_close(shared_memory->sem_users);
    sem_unlink("SEM_COMPRAS");
    sem_unlink("SEM_USERS");

    kill(shared_memory->refresh_pid, SIGSEGV);
    // Remove shared_memory
    if (shmdt(shared_memory) == -1) {
        perror("acoplamento impossivel");
    }
    if (shmctl(shm_id, IPC_RMID, 0) == -1) {
        perror("destruicao impossivel");
    }
    close(fd);
    close(s);

    exit(0);
}
