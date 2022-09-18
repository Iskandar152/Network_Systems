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
    int num_reads;
    char *hostname;
    char buf[BUFFER_SIZE];
    char reply_buf[BUFFER_SIZE];
    char packet_recv[BUFFER_SIZE];
    char put_filename[30];
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
      n = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&serveraddr, serverlen);

      if(n < 0){
        error("ERROR in sendto");
      }

      n = recvfrom(sockfd, buf, BUFFER_SIZE,0,(struct sockaddr *)&serveraddr,&serverlen);
      
      if(n < 0){
        error("ERROR in recvfrom");
      }
      
      switch(command){
        case 1:
          printf("Current Directory: %s", buf);
          bzero(buf,BUFFER_SIZE);
          break;
        
        case 2:
          printf("%s", buf);
          exit(1);
          break;
        
        case 3:
        {
          size_t read;
          size_t written;
          unsigned char input_buff[BUFFER_SIZE];
          bzero(input_buff,BUFFER_SIZE);
          char int_converter[10];

          bzero(put_filename,30);
          n = recvfrom(sockfd, put_filename, 30,0,(struct sockaddr *)&serveraddr,&serverlen);
          printf("%d\n",n);
          printf("%s\n",put_filename);
          
          if(access(put_filename,R_OK) != 0){
              printf("File not found\n");
              reply_buf[0] = '1';
              n = sendto(sockfd, reply_buf, 1, 0, (struct sockaddr *)&serveraddr, serverlen);
              break;
          }
          else{
              reply_buf[0] = '0';
              n = sendto(sockfd, reply_buf, 1, 0, (struct sockaddr *)&serveraddr, serverlen);
          }

          
          FILE *input;
          input = fopen(put_filename,"rb");
          char written_convert[10];
          bzero(int_converter,10);
          
          read = fread(input_buff, 1, sizeof input_buff, input);
          printf("READ: %d\n",read);
          snprintf(int_converter, 10, "%d", read);
          printf("INT CONVERT: %s\n", int_converter);
          n = sendto(sockfd, int_converter, strlen(int_converter), 0, (struct sockaddr *)&serveraddr, serverlen);
          n = sendto(sockfd, input_buff, read, 0, (struct sockaddr *)&serveraddr, serverlen);
          printf("N SENT: %d\n",n);
          n = recvfrom(sockfd, written_convert, 10,0,(struct sockaddr *)&serveraddr,&serverlen);
          written = atoi(written_convert);
          printf("WRITTEN: %d\n", written);
          /*
          do{
            printf("???\n");
            read = fread(input_buff, 1, sizeof(input_buff), input);
            printf("?\n");
            snprintf(int_converter, 10, "%d", read);
            printf("?\n");
            n = sendto(sockfd, int_converter, strlen(int_converter), 0, (struct sockaddr *)&serveraddr, serverlen);
            printf("?\n");
            if(read){
              
            }
            else{
              written = 0;
            }


          } while((read > 0) && (written == read));
          */

          


          
          
          break;
        }
        case 4:
        {
          char file_name[15];
          printf("GET\n");
          if(strncmp(buf,"1",1) == 0){
            printf("File does not exist\n");
            printf("%s\n", buf);
            bzero(buf,BUFFER_SIZE);
            break;
          }
          bzero(buf,BUFFER_SIZE);
          //Receive file name 
          
          n = recvfrom(sockfd, buf, 15,0,(struct sockaddr *)&serveraddr,&serverlen);
          strcpy(file_name,buf);
          bzero(buf,BUFFER_SIZE);
          
          //Receive number of reads required 
          n = recvfrom(sockfd, buf, BUFFER_SIZE,0,(struct sockaddr *)&serveraddr,&serverlen);
          printf("NUMREADS: %s\n", buf);

          num_reads = atoi(buf);
          FILE *test_file;
          test_file = fopen(file_name,"wb+");
          for(int i = 0; i < num_reads; i++){
            n = recvfrom(sockfd, packet_recv, BUFFER_SIZE,0,(struct sockaddr *)&serveraddr,&serverlen);
            printf("SIZE: %d\n", n);
            fwrite(packet_recv, BUFFER_SIZE, BUFFER_SIZE, test_file);
            for(int j = 0; j < BUFFER_SIZE; j++){     
              printf("\x1B[31m%d",j);
              printf("\x1B[0m%x",packet_recv[j]);
            }
            printf("\n");
            //bzero(packet_recv,BUFFER_SIZE);
            
          }

          fclose(test_file);
        }
          break;

        
        case 5:
          buf[1] = '\0';
          if(strncmp(buf,"0",1) == 0){
            printf("File Removed Successfully\n");
          }
          else{
            printf("File not found\n");
          }
          break;

        default:
          printf("INVALID COMMAND\n");

        

      }
      


    }

    return 0;
}
