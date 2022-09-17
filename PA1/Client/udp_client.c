/* 
 * udpclient.c - A simple UDP client
 * usage: udpclient <host> <port>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

#define BUFFER_SIZE 1024

/* 
 * error - wrapper for perror
 */
void error(char *msg) {
    perror(msg);
    exit(0);
}

int main(int argc, char **argv) {
    int sockfd, portno, n;
    int serverlen;

    char *hostname;
    char buf[BUFFER_SIZE];

    struct sockaddr_in serveraddr;
    struct hostent *server;


    /* check command line arguments */
    if (argc != 3) {
       fprintf(stderr,"usage: %s <hostname> <port>\n", argv[0]);
       exit(0);
    }
    hostname = argv[1];
    portno = atoi(argv[2]);

    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(0);
    }

    /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
	  (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);





    /* get a message from the user */
    while(1){
      int command = 0; 
      bzero(buf,BUFFER_SIZE);
      
      printf("Please Enter a Command: ");
      fgets(buf, BUFFER_SIZE, stdin);

      //Checking which command was placed in 
      if(strncmp(buf,"ls",2) == 0) command = 1;
      else if(strncmp(buf,"exit",4) == 0) command = 2;
      else if(strncmp(buf,"put",3) == 0) command = 3;
      else if(strncmp(buf,"get",3) == 0) command = 4;
      else if(strncmp(buf,"delete",6) == 0) command = 5;

      
      //Send message
      serverlen = sizeof(serveraddr);
      n = sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);

      if(n < 0){
        error("ERROR in sendto");
      }

      n = recvfrom(sockfd, buf, BUFFER_SIZE,0,&serveraddr,&serverlen);
      
      if(n < 0){
        error("ERROR in recvfrom");
      }
      
      switch(command){
        case 1:
          printf("Current Directory: %s", buf);
          break;
        
        case 2:
          printf("%s", buf);
          exit(1);
          break;
        
        case 3:
          printf("put\n");
          break;
        
        case 4:
          printf("get\n");
          break;

        
        case 5:
          printf("delete\n");
          break;

        default:
          printf("INVALID COMMAND\n");

        

      }
      


    }

    return 0;
}
