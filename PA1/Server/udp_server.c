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
        printf("server received %ld/%d bytes: %s\n", strlen(buf), n, buf);

        if(n < 0){
            error("ERROR in sendto");
        }



        bzero(reply_buf,BUFFER_SIZE);

        /*
        COMMAND HANDLING
        ==================================================================================================
        */
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
            strcpy(reply_buf, "Goodbye!\n");
            //Sending a goodbye issue to client 
            n = sendto(sockfd, reply_buf, BUFFER_SIZE, 0, (struct sockaddr *) &clientaddr, clientlen);
        }
        else if(strncmp(buf,"delete", 6) == 0){
            //Directory holds the entire path to the file to delete 
            char directory[BUFFER_SIZE];
            //Getting the current directory 
            getcwd(directory, sizeof(directory));

                
            //File name without all the path information 
            char del_file[BUFFER_SIZE];
            bzero(del_file,BUFFER_SIZE);
            bzero(reply_buf,BUFFER_SIZE);
            //7 is the size of the word "delete" plus a space 
            for(int i = 7; i < strlen(buf) - 1; i++){
                del_file[i - 7] = buf[i];
            }
            //Making sure there's an end line at the end 
            del_file[strlen(buf) - 1] = '\0';
            //Putting together the directory plus file for full path 
            strcat(directory,del_file);
            //Remove the file (checks if the removal was successful)
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
                n = sendto(sockfd, pure_file_name, strlen(pure_file_name), 0, (struct sockaddr *) &clientaddr, clientlen);
            }
            else{
                n = sendto(sockfd, put_file, strlen(put_file), 0, (struct sockaddr *) &clientaddr, clientlen);
            }

            //Does file exist to copy from to server 
            bzero(buf,BUFFER_SIZE);
            n = recvfrom(sockfd,buf,BUFFER_SIZE,0, (struct sockaddr *) &clientaddr,&clientlen);


            //===================================================================================================================================
            
            FILE *output;
            output = fopen(put_file,"wb");
            //Stores the read data received from client 
            char read_convert[10];
            //int_converter will hold the amount of written data to be sent to the client 
            char int_converter[10];
            //Clearing the buffers just in case 
            bzero(int_converter,10);
            bzero(read_convert,10);

            //Do while is used because we need to try fetching data atleast once 
            //to determine if we continue the loop 
            //Used this as a source: https://stackoverflow.com/questions/5263018/copying-binary-files
            do{
                //Client is reading in file, read in how much data the client got from the file 
                n = recvfrom(sockfd,read_convert,10,0, (struct sockaddr *) &clientaddr,&clientlen);
                //Converting the string we received into an integer
                read = atoi(read_convert);
                //Receive the actual packet (max size is 1024 bits);
                n = recvfrom(sockfd,output_buffer,BUFFER_SIZE,0, (struct sockaddr *) &clientaddr,&clientlen);

                if(read){
                    //If we actually were able to get some data, we start writing this data 
                    //into a file of the same name the client is sending 
                    written = fwrite(output_buffer, 1, read, output);
                    //snprintf is used to convert the integer into a string to be sent over
                    snprintf(int_converter,10,"%ld",written);
                    //Send how much data was written in (Ideally will be equal to the number of data the client read in);
                    n = sendto(sockfd, int_converter, strlen(int_converter), 0, (struct sockaddr *) &clientaddr, clientlen);
                }
                else{
                    //If read data is 0, we set written to 0 as well. We will be leaving the loop at this point 
                    written = 0;
                }
                bzero(output_buffer,BUFFER_SIZE);
                bzero(int_converter,10);
                bzero(read_convert,10);

            } while((read > 0) && (written == read));
            
            //Close the file 
            fclose(output);
        }
        else if(strncmp(buf,"get", 3) == 0){

            /*
             * Setting up the directory from which to get the file 
             * Setting up buffers and other variables. Similar to 
             * the put routine above
            */
            size_t read;
            size_t written;
            unsigned char input_buff[BUFFER_SIZE];
            bzero(input_buff,BUFFER_SIZE);
            char int_converter[10];

            char directory[BUFFER_SIZE];
            bzero(directory,BUFFER_SIZE);
            getcwd(directory, sizeof(directory));
            directory[strlen(directory)] = '/';
            char get_file[BUFFER_SIZE];
            bzero(get_file,BUFFER_SIZE);
            bzero(reply_buf,BUFFER_SIZE);
            
            //Reading in data after the put command ('put' is 3 characters plus the space)
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
                //Send confirmation
                reply_buf[0] = '0';
                n = sendto(sockfd, reply_buf, 1, 0, (struct sockaddr *) &clientaddr, clientlen);
                
                //Send file name
                n = sendto(sockfd, get_file, 15, 0, (struct sockaddr *) &clientaddr, clientlen);
                n = sendto(sockfd, get_file, 15, 0, (struct sockaddr *) &clientaddr, clientlen);
                printf("FILE IS ABLE TO BE READ\n");

                FILE *input;
                input = fopen(directory,"rb");
                char written_convert[10];
                bzero(int_converter,10);

                do{
                    read = fread(input_buff, 1, sizeof(input_buff), input);
                    snprintf(int_converter, 10, "%ld", read);
                    n = sendto(sockfd, int_converter, strlen(int_converter), 0, (struct sockaddr *) &clientaddr, clientlen);
                    n = sendto(sockfd, input_buff, read, 0, (struct sockaddr *) &clientaddr, clientlen);

                    if(read){
                        n = recvfrom(sockfd,written_convert,10,0, (struct sockaddr *) &clientaddr,&clientlen);
                        written = atoi(written_convert);
                    }
                    else{
                        written = 0;
                    }
                    bzero(written_convert,10);
                    bzero(int_converter,10);

                } while((read > 0) && (written == read));
                

                
                fclose(input);
            }

            
        }
        else{
            printf("Invalid Command\n");
        }

        
    }



    return 0; 
}