// stock_server {PORTO_BOLSA} {PORTO_CONFIG} {ficheiro configuração}
// Server<->Cliente TCP
// Server<->Consola_Admin UDP
// TCP - nc -v localhost 9000
// UDP - nc -u -4 localhost 9001

#include "func.h"

#define BUF_SIZE 1024

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

    config(path, shared_memory);

    pid_t pid;

    sem_unlink("MUTEX_COMPRAS");
    // sem_unlink("MUTEX_USER2");
    // sem_unlink("MUTEX_LOGIN");
    shared_memory->mutex_compras = sem_open("MUTEX_COMPRAS", O_CREAT | O_EXCL, 0700, 1);
    // shared_memory->mutex_user2 = sem_open("MUTEX_USER2", O_CREAT | O_EXCL, 0700, 0);
    // shared_memory->mutex_login = sem_open("MUTEX_LOGIN", O_CREAT | O_EXCL, 0700, 0);
    // int i = 0;

    pid = fork();
    if (pid == 0) {

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
            }
            exit(0); // retirar este exit, so no caso de o login nao ser feito para ja
        }
        exit(0);
    }

    char buffer[BUF_SIZE];
    while (login_admin(s, shared_memory) == -1)
        ;
    // Espera recepção de mensagem (a chamada é bloqueante)
    if ((recv_len = recvfrom(s, buffer, BUF_SIZE, 0, (struct sockaddr *)&admin_addr, (socklen_t *)&slen)) == -1) {
        erro("Erro no recvfrom");
    }
    // if ((send_len = sendto(s, palavra, strlen(palavra), 0, (struct sockaddr *)&si_minha, slen)) == -1) {
    buffer[recv_len - 1] = '\0';

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

    while(wait(NULL)!=1 || errno!=ECHILD){
        printf("wainted for a child to finish\n");
    }
    //fechar os sockets no processo main!
    close(fd);
    close(s);

    return 0;
}