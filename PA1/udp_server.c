/*
UDP Server by Iskandar Shoyusupov
Used udp_server from class as a tempelate 
*/ 

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> 

#define BUFFER_SIZE 1024 

void error(char *msg){
    perror(msg);
    exit(1);
}

int main(int argc, char **argv){
    
    int sockfd;   //Create a UDP socket 
    int portno;   //Port entered by user to listen to
    int optval;   //Flag Value for setsockopt
    int clientlen; //Size of client's address in bytes 
    int n; //Message byte size 

    char buf[BUFFER_SIZE];
    char reply_buf[BUFFER_SIZE];
    char *hostaddrp; 

    struct sockaddr_in serveraddr; 
    struct sockaddr_in clientaddr;
    struct hostent *hostp;

    //Check that the amount of arguments is exactly 2 
    if(argc != 2){
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
    
    //Save clients port number 
    portno = atoi(argv[1]);

    /*
     * Creating the parent socket 
     *
     * Socket Structure: socket(int Domain, int Type, int Protocol)
     * @Domain: AF_INET signifies IPV4 Domain
     * @Type: SOCK_DGRAM set to UDP 
     * @Protocol:  0 
     */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0); 
    
    if(sockfd < 0){
        error("ERROR OPENING SOCKET");
    }


    /*
     * As described in template, setsockopt is a debugging trick 
     * to allow rerunning the server after we kill it. 
     * Eliminates "ERROR on binding: Address already in use" error 
     * 
     * Use: int setsockopt(int socket, int level, int option_name,
     *                     const void *option_value, socklen_t option_len);
     */
    optval = 1; 
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));


    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET; 
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    serveraddr.sin_port = htons((unsigned short)portno);

    if(bind(sockfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0){
        error("ERROR ON BINDING");
    }

    clientlen = sizeof(clientaddr);
    while(1){
        bzero(buf, BUFFER_SIZE);
        n = recvfrom(sockfd,buf,BUFFER_SIZE,0, (struct sockaddr *) &clientaddr,
            &clientlen);
        
        if(n < 0){
            error("ERROR IN recvfrom");
        }

        hostp = gethostbyaddr((const char*)&clientaddr.sin_addr.s_addr,
                              sizeof(clientaddr.sin_addr.s_addr), AF_INET);
        if(hostp == NULL){
            error("ERROR ON gethostbyaddr");
        }
        hostaddrp = inet_ntoa(clientaddr.sin_addr);
        if(hostaddrp == NULL){
            error("ERROR ON inet_ntoa\n");
        }
        printf("server received datagram from %s (%s)\n",hostp->h_name, hostaddrp);
        printf("server received %d/%d bytes: %s\n", strlen(buf), n, buf);

        //n = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *) &clientaddr, clientlen);
        if(n < 0){
            error("ERROR in sendto");
        }
        bzero(reply_buf,BUFFER_SIZE);
        if(strncmp(buf,"ls",2) == 0){
            printf("ls Processed\n");
            FILE *data_in;
            data_in = popen("ls", "r");
            if(data_in == NULL){
                printf("NOTHING\n");
            }
            while(fgets(reply_buf,BUFFER_SIZE,data_in) != NULL){
               printf("%s",reply_buf); 
               printf("%d",strlen(reply_buf));
            };
            printf("\n");
            
            pclose(data_in);
            n = sendto(sockfd, reply_buf, BUFFER_SIZE, 0, (struct sockaddr *) &clientaddr, clientlen);
        }
        else if(strncmp(buf,"exit", 4) == 0){
            printf("Exit Processed\n");
            
            strcpy(reply_buf, "Goodbye!\n");
            n = sendto(sockfd, reply_buf, BUFFER_SIZE, 0, (struct sockaddr *) &clientaddr, clientlen);
        }
        else if(strncmp(buf,"delete", 6) == 0){
            printf("Delete Processed\n");
        }
        else if(strncmp(buf,"put", 3) == 0){
            printf("Put Processed\n");
        }
        else if(strncmp(buf,"get", 3) == 0){
            printf("Get Processed\n");
        }
        else{
            printf("Invalid Command\n");
        }

        
    }



    return 0; 
}