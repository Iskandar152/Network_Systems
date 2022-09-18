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

        if(n < 0){
            error("ERROR in sendto");
        }



        bzero(reply_buf,BUFFER_SIZE);


        if(strncmp(buf,"ls",2) == 0){
            FILE *data_in;
            data_in = popen("ls", "r");

            if(data_in == NULL){
                printf("No Files\n");
            }

            char output[BUFFER_SIZE];
            int location = 0;
            
            while(fgets(reply_buf,BUFFER_SIZE,data_in) != NULL){
                for(int i = 0; i < strlen(reply_buf) - 1; i++){
                    output[location] = reply_buf[i];
                    location += 1;
                }
                output[location] = ' ';
                location += 1;
            };
            output[location] = '\n';
            output[location + 1] = '\0';        
            printf("local directory send: %s", output);
            pclose(data_in);
            n = sendto(sockfd, output, BUFFER_SIZE, 0, (struct sockaddr *) &clientaddr, clientlen);
        }
        else if(strncmp(buf,"exit", 4) == 0){
            printf("Exit Processed\n");
            
            strcpy(reply_buf, "Goodbye!\n");
            n = sendto(sockfd, reply_buf, BUFFER_SIZE, 0, (struct sockaddr *) &clientaddr, clientlen);
        }
        else if(strncmp(buf,"delete", 6) == 0){
            char directory[BUFFER_SIZE];
            getcwd(directory, sizeof(directory));

                
            
            char del_file[BUFFER_SIZE];
            bzero(del_file,BUFFER_SIZE);
            bzero(reply_buf,BUFFER_SIZE);

            for(int i = 7; i < strlen(buf) - 1; i++){
                del_file[i - 7] = buf[i];
            }
           
            del_file[strlen(buf) - 1] = '\0';

            strcat(directory,del_file);
            if(!remove(del_file)){
                reply_buf[0] = '0';
                printf("File %s removed successfully\n", del_file);
                n = sendto(sockfd, reply_buf, 1, 0, (struct sockaddr *) &clientaddr, clientlen);
            }
            else{
                reply_buf[0] = '1';
                printf("File doesn't exit\n");
                n = sendto(sockfd, reply_buf, 1, 0, (struct sockaddr *) &clientaddr, clientlen);
            }
        }
        else if(strncmp(buf,"put", 3) == 0){
            printf("Put Processed\n");

            /*
            Set up directory, buffers, and other variables
            ======================================================================================================================================
            */
            size_t read;
            size_t written;
            unsigned char output_buffer[BUFFER_SIZE];
            
            char directory[BUFFER_SIZE];
            bzero(directory,BUFFER_SIZE);     //Directory needs to be zero'd out, otherwise there is uncertain behavior with uncleared data 
            char put_file[30];
            getcwd(directory, sizeof(directory)); 
            directory[strlen(directory)] = '/';
            bzero(put_file,30);
            char pure_file_name[30];
            bzero(pure_file_name,30);
            int subdir = 0;                   //Is file entered a subdirectory 
            //Copy over everything after the put command and the first space 
            for(int i = 4; i < strlen(buf) - 1; i++){
                put_file[i - 4] = buf[i];
            }

            //Find the actual file name without any directory details 
            for(int i = strlen(put_file); i >= 0; i--){
                if(put_file[i] == '/'){
                    subdir = 1;
                    for(int j = (i + 1); j < strlen(put_file); j++){
                        pure_file_name[j - i - 1] = put_file[j];
                    }
                }
            }
            printf("%s\n",pure_file_name);

            //Add an end line at the end of our file name, just in case 
            put_file[strlen(buf)] = '\0';
            strcat(directory, put_file);
            n = sendto(sockfd, put_file, strlen(put_file), 0, (struct sockaddr *) &clientaddr, clientlen);   //this is really bugging me, one sendto doesn't work for some reason
                                                                                                             //this shouldn't be necessary???
            //If file is in a subdirectory structure, then we send back the pure file name to the client 
            //Otherwise we already have the pure file name to send in put_file
            if(subdir){
                printf("SENT FILE NAME UNDER ABNORMAL DIR\n");
                n = sendto(sockfd, pure_file_name, strlen(pure_file_name), 0, (struct sockaddr *) &clientaddr, clientlen);
            }
            else{
                printf("SENT: %s\n",put_file);
                n = sendto(sockfd, put_file, strlen(put_file), 0, (struct sockaddr *) &clientaddr, clientlen);
            }

            //Does file exist to copy from to server 
            bzero(buf,BUFFER_SIZE);
            n = recvfrom(sockfd,buf,BUFFER_SIZE,0, (struct sockaddr *) &clientaddr,&clientlen);
            printf("BUFFER:%s\n",buf);


            printf("DIRECTORY: %s\n", directory);

            //===================================================================================================================================
            
            FILE *output;
            output = fopen(put_file,"wb");
            char read_convert[10];
            char int_converter[10];
            bzero(int_converter,10);
            bzero(read_convert,10);


            n = recvfrom(sockfd,read_convert,10,0, (struct sockaddr *) &clientaddr,&clientlen);
            read = atoi(read_convert);
            n = recvfrom(sockfd,output_buffer,BUFFER_SIZE,0, (struct sockaddr *) &clientaddr,&clientlen);
            printf("N SENT: %d\n",n);
            written = fwrite(output_buffer, 1, read, output);
            printf("WRITTEN: %d\n", written);
            snprintf(int_converter,10,"%d",written);

            n = sendto(sockfd, int_converter, strlen(int_converter), 0, (struct sockaddr *) &clientaddr, clientlen);
            /*
            do{
                printf("NUM:%s\n",int_convert);
                n = recvfrom(sockfd,int_convert,10,0, (struct sockaddr *) &clientaddr,&clientlen);
                printf("Num: %s\n",int_convert);

            } while((read > 0) && (written == read));
            */
            fclose(output);
        }
        else if(strncmp(buf,"get", 3) == 0){

            /*
             *Setting up the directory from which to get the file 
            */

            char directory[BUFFER_SIZE];
            bzero(directory,BUFFER_SIZE);
            getcwd(directory, sizeof(directory));
            directory[strlen(directory)] = '/';
            char get_file[BUFFER_SIZE];
            bzero(get_file,BUFFER_SIZE);
            bzero(reply_buf,BUFFER_SIZE);
            
            
            for(int i = 4; i < strlen(buf) - 1; i++){
                    get_file[i - 4] = buf[i];
            }
            get_file[strlen(buf) - 1] = '\0';
            
            strcat(directory,get_file);
            
            printf("Location: %s\n", directory);
            
            if(access(directory,R_OK) != 0){
                printf("File not found\n");
                reply_buf[0] = '1';
                n = sendto(sockfd, reply_buf, 1, 0, (struct sockaddr *) &clientaddr, clientlen);
            }
            else{
                /*
                Sending packets with file 
                */

                //Send confirmation
                reply_buf[0] = '0';
                n = sendto(sockfd, reply_buf, 1, 0, (struct sockaddr *) &clientaddr, clientlen);
                
                //Send file name
                n = sendto(sockfd, get_file, 15, 0, (struct sockaddr *) &clientaddr, clientlen);

                printf("FILE IS ABLE TO BE READ\n");

                FILE *f_read;
                f_read = fopen(directory,"rb");
                
                fseek(f_read, 0L, SEEK_END);
                int file_size = ftell(f_read);
                int reads_required;  
                char reads_req_str[12];
                
                fseek(f_read, 0L, SEEK_SET);
                
                if(file_size <= 1024){
                    reads_required = 1;
                }
                else{
                    //This helps me round up
                    //Source: https://stackoverflow.com/questions/2745074/fast-ceiling-of-an-integer-division-in-c-c
                    reads_required = ((file_size + 1023) / 1024);
                }
                sprintf(reads_req_str, "%d", reads_required);
                printf("STR FORM: %s\n",reads_req_str);
                printf("SIZE:%d\n", strlen(reads_req_str));
                printf("FILE SIZE: %d\n\n",file_size);
                printf("READS REQUIRED: %d\n",reads_required);
                char packet[BUFFER_SIZE];
                bzero(packet,BUFFER_SIZE);
                
                n = sendto(sockfd, reads_req_str, 12, 0, (struct sockaddr *) &clientaddr, clientlen);
                
                for(int i = 0; i < reads_required; i++){

                    fread(packet,BUFFER_SIZE,1,f_read);
                    printf("LENGTH: %d\n", strlen(packet));
                    for(int i = 0; i < BUFFER_SIZE; i++){
                        printf("\x1B[31m%d",i);
                        printf("\x1B[0m%x",packet[i]);
                    }
                    n = sendto(sockfd, packet, BUFFER_SIZE, 0, (struct sockaddr *) &clientaddr, clientlen);
                }
            
                printf("\n\n\n");
                
                fclose(f_read);
            }

            
        }
        else{
            printf("Invalid Command\n");
        }

        
    }



    return 0; 
}