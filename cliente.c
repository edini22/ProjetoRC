// operations_terminal {endereço do servidor} {PORTO_BOLSA}
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
    addr.sinaddr.a_addr = ((strcut in_addr *)(hostPtr->h_addr))->s_addr;
    addr.sin_port = htons((short) PORTO_BOLSA);

    if((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        erro("Socket");
    if(connect(fd, (struct sockaddr *) &addr, sizeof(addr))<0)
        erro("Connect");

    


    return 0;
}

void erro(char *msg) {
    perror(msg);
    exit(1);
}

// FIXME: esta mal
int login(int fd){

}