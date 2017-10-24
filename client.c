#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>

int sock;

void gestore(int signo) {

    printf("%d: termino", getpid());
    shutdown(sock, 0);
    exit(EXIT_SUCCESS);


}

int main(int argc, char *argv[]) {
    int nread, port;
    int status; //mi serve per la wait finale del padre

    int i = 0; //mi serve in un for alla fine

    int pid_son1;
    int pid_son2;

    char msg[256];
    
    
    struct hostent *host;
    struct sockaddr_in servaddr;

    if (argc != 3) {
        printf("Error:%s destAddress serverPort\n", argv[0]);
        exit(1);
    }

    memset((char *) &servaddr, 0, sizeof (struct sockaddr_in));
    servaddr.sin_family = AF_INET;

    host = gethostbyname(argv[1]);
    if (host == NULL) {
        printf("%s non trovato\n", argv[1]);
        exit(1);
    }

    nread = 0;
    while (argv[2][nread] != '\0') {
        if ((argv[2][nread] < '0') || (argv[2][nread] > '9')) {
            printf("Secondo argomento non intero\n");
            exit(2);
        }
        nread++;
    }

    port = atoi(argv[2]);
    if (port < 1024 || port > 65535) {
        printf("Porta invalida");
        exit(2);
    }

    servaddr.sin_addr.s_addr = ((struct in_addr*) (host->h_addr))->s_addr;
    servaddr.sin_port = htons(port);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Errore apertura socket");
        exit(1);
    }

    if (connect(sock, (struct sockaddr *) &servaddr, sizeof (struct sockaddr))
            < 0) {
        perror("Errore connessione col sdestinatario");
        exit(1);
    }
    printf("\nSocket creata\nConnessione col destinatario eseguita\n\n");

    //------------------------------------------------------------------------------------------------------------------//da qui inizia la parte vera e propria


    if ((pid_son1 = fork()) == 0) { //Primo figlio: si occupa della lettura dalla socket

        signal(SIGUSR1, gestore);

        shutdown(sock, 1); //chiudo socket in scrittura

        while ((nread = read(sock, msg, sizeof (msg))) > 0) { //legge PROBLEMA se arriva un EOF sulla socket --- non me ne sono ancora occupato
            write(1, msg, nread);
        }

        exit(EXIT_SUCCESS);
    }
    if ((pid_son2 = fork()) == 0) { //PSecondo figlio: si occupa della scrittura dalla socket

        shutdown(sock, 0); //chiudo socket in lettura

        while (gets(msg)) {
            write(sock, msg, sizeof (msg)); //leggo fino a EOF
        }

        kill(pid_son1, SIGUSR1); // se ricevo EOF, mando un segnale al primo figlio (suppongo sia ancora vivo) e lo uccido
        shutdown(sock, 1);
        exit(EXIT_SUCCESS);

    }

    for (i = 0; i < 2; i++) {  //padre aspetta terminazione figli
        wait(&status);
    }

    close(sock);
    printf("\nTerminazione.\n");
    exit(0);
}
