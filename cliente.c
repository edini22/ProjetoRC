// operations_terminal {endereço do servidor} {PORTO_BOLSA}
// ./operations_terminal 127.0.0.1 9000
// Server<->Cliente TCP

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUF_SIZE 1024
int fd;

void erro(char *msg);

int login(int fd){
    char buffer[BUF_SIZE];

    // Login: Username:
    read(fd, buffer, BUF_SIZE);
    printf("%s", buffer);
    fflush(stdout);
    memset(buffer, 0, BUF_SIZE);

    // Send username
    scanf("%s", buffer);
    write(fd, buffer, BUF_SIZE);
    fflush(stdout);
    memset(buffer, 0, BUF_SIZE);

    // Password:
    read(fd, buffer, BUF_SIZE);
    printf("%s", buffer);
    fflush(stdout);
    memset(buffer, 0, BUF_SIZE);

    // Send password
    scanf("%s", buffer);
    write(fd, buffer, BUF_SIZE);
    fflush(stdout);
    memset(buffer, 0, BUF_SIZE);

    // Verify login
    char erro_msgs[2][100] = {"\nO Username nao existe", "\nPassword incorreta"};
    read(fd, buffer, BUF_SIZE);
    if (!strcmp(buffer,erro_msgs[0]) || !strcmp(buffer,erro_msgs[1])){
        printf("Username ou password errada\n");
        return -1;
    } else{
        printf("Login efetuado com sucesso!\n");
    }


    return 0;
}

int main(int argc, char **argv){

    struct sockaddr_in addr;
    struct hostent *hostPtr;
    
    if (argc!=3){
        erro("Parametros mal especificados. Exemplo:\nstock_server {PORTO_BOLSA} {PORTO_CONFIG} {ficheiro configuração}");
    }

    // leitura dos parametros
    
    char *ponteiro;
    char endereco[50];
    strcpy(endereco, argv[1]);
    int PORTO_BOLSA = (int) strtol(argv[2], &ponteiro, 10);

    // Conexao TCP
    if((hostPtr = gethostbyname(endereco))==0){
        erro("Endereco errado");
    }
    bzero((void *) &addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ((struct in_addr *)(hostPtr->h_addr))->s_addr;
    addr.sin_port = htons((short) PORTO_BOLSA);

    if((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        erro("Socket");
    if(connect(fd, (struct sockaddr *) &addr, sizeof(addr))<0)
        erro("Connect");

    login(fd);

    while(1){
        //TODO: 
        sleep(2);
        exit(0);
    }

    return 0;
}

void erro(char *msg) {
    perror(msg);
    exit(1);
}
