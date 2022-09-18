#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> 

int main(int argc, char **argv){
    FILE *exein, *exeout;

    exein = fopen("udp_serverr", "rb");
    if (exein == NULL) {
        /* handle error */
        perror("file open for reading");
        exit(EXIT_FAILURE);
    }
    exeout = fopen("stuff", "wb");
    if (exeout == NULL) {
        /* handle error */
        perror("file open for writing");
        exit(EXIT_FAILURE);
    }

    size_t n, m;
    unsigned char buff[8192];
    do {
        n = fread(buff, 1, sizeof buff, exein);
        if (n) m = fwrite(buff, 1, n, exeout);
        else   m = 0;
    } while ((n > 0) && (n == m));
    if (m) perror("copy");

    if (fclose(exeout)) perror("close output file");
    if (fclose(exein)) perror("close input file");

    return 0;
}