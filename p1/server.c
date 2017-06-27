#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/fcntl.h>
#include <time.h>
#define MAX_LENGTH 4096
#define TRUE 1
#define FALSE 0

void error(char *message)
{
    perror(message);
    exit(1);
}

void Get_time(char timeout[]){
    time_t rawtime;
    time ( &rawtime );
    strncpy(timeout,ctime(&rawtime)+4,15);

}
void Read_file(char filename[], struct sockaddr_in client_address, char output[], int servsocket, char *path){
    char response[MAX_LENGTH];
    memset(response, 0, MAX_LENGTH*sizeof(char));
    int bytes_read;
    long int fsize;
    FILE *input = fopen(filename, "r");

    if(input==NULL){
        strcat(output,"HTTP/1.0 404 Not Found");
        strcpy(response,"\nHTTP/1.0 404 Not Found\n");
        sendto(servsocket,response,strlen(response),0,(struct sockaddr *)&client_address, sizeof(struct sockaddr));
    }else{
        strcat(output,"HTTP/1.0 200 OK; ");

        //Determine the requested file siz
        fseek(input, 0L , SEEK_END);
        fsize = ftell(input);
        char *file=malloc(fsize + 1);
        fseek(input, 0L, SEEK_SET);

        //Reads the file into the buffer
        bytes_read=fread(file, sizeof(char), fsize, input);
        fclose(input);
        strcpy(response, "\nHTTP/1.0 200 OK\n\n");
        strcat(response, file);
        strcat(response, "\r\n\r\n\0");
        free(file);

        //Used for sending single message
        int BUFFER_SIZE=512;
        if(bytes_read<=BUFFER_SIZE)
            sendto(servsocket,response,strlen(response),0,(struct sockaddr *)&client_address, sizeof(struct sockaddr));
        else {
            //Used for sending long, multi-packet messages
            int index=0;
            while(index<bytes_read){
                sendto(servsocket,&response[index],BUFFER_SIZE,0,(struct sockaddr *)&client_address, sizeof(struct sockaddr));
                index+=BUFFER_SIZE;
            }
        }

        strcat(output,path);
        strcat(output,filename);
    }

}
void processRequest(char recvbuff[],char *path, int servsocket, char output[], struct sockaddr_in client_address){
    char response[MAX_LENGTH];
    char *pathIndex = strchr(recvbuff, '/');// filename
    char *httpIndex = strchr(pathIndex, ' ');// HTTP/1.0

    strcpy(output,"GET / HTTP/1.0; " );
    if(strncmp(recvbuff, "GET /", 5) !=0 || strncmp(httpIndex," HTTP/1.0",9)!=0){
        strcat(output, "HTTP/1.0 400 Bad Request");
        strcpy(response,"\nHTTP/1.0 400 Bad Request\n");
        sendto(servsocket,response,strlen(response),0,(struct sockaddr *)&client_address, sizeof(struct sockaddr));
        //printf("%s", output);
    }else {
        //get filename
        int pathN;
        pathN = httpIndex-pathIndex  ;
        char filename[MAX_LENGTH];
        if (pathN==1){
            strcpy(filename, "index.html");
        }else{
            //printf("%d\n", pathN);
            strncpy(filename, pathIndex + 1, pathN-1);
            filename[pathN-1]='\0';
        }
        //printf("%s\n", filename);
        Read_file(filename, client_address, output, servsocket,path);

    }

}
int main(int argc, char *argv[])
{

    fd_set readfds;
    int servsocket, bytes_read;
    unsigned int address_length;
    char output[MAX_LENGTH],recvbuff[MAX_LENGTH];
    struct sockaddr_in server_address , client_address;
    int port;
    char *path;

    //check arg
    if(argc == 3){
        //if input the port number and the path, go to the path document and use the input file
        port = atoi(argv[1]);
        path = argv[2];
        //Checking the existence of the given directory
        if(chdir(path) == -1){
            error("Invalid file path.\n");
        }
    }else{
        // if too many input, exit
        error("Input Error.\n");

    }

    //Creating/Binding Datagram socket
    if ((servsocket = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        error("socket() error");

    //Sets up Server Address Data
    memset(&server_address,0,sizeof(struct sockaddr_in));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = INADDR_ANY;

    //Binds the socket to the data we just set

    if (bind(servsocket,(struct sockaddr *)&server_address, sizeof(struct sockaddr)) == -1)
        error("bind() error");

    //Configure socket to reuse ports
    int optval=TRUE;
    setsockopt(servsocket, SOL_SOCKET, SO_REUSEADDR,(char*)&optval,sizeof(optval));

    address_length = sizeof(struct sockaddr);

    printf("\nsws is running on UDP port %d and serving %s\n",port,path);
    printf("press ‘q’ to quit ...\n");
    fflush(stdout);
    //Loop to receive the requests and checking “q” button
    while (1)
    {
        FD_ZERO(&readfds);
        FD_SET(0, &readfds);
        FD_SET(servsocket, &readfds);

        if (select(servsocket + 1, &readfds, 0,/*NULL,*/ 0, 0) == -1){
            perror("Failed to select!"); // error occurred in select()
            break;
        }
        //If the socket got something from client
        if (FD_ISSET(servsocket, &readfds)) //if set to read
        {
            char timeout[MAX_LENGTH];
            Get_time(timeout);
            bytes_read = recvfrom(servsocket,recvbuff,MAX_LENGTH,0,(struct sockaddr *)&client_address, &address_length);
            recvbuff[bytes_read] = '\0';

            //Processing the received request (String Processing)
            processRequest(recvbuff, path, servsocket, output,client_address);

            //output in Server
            printf("%s %s:%d %s\n",timeout,inet_ntoa(client_address.sin_addr),ntohs(client_address.sin_port), output);
            //Clears buffer for next connection
            memset(recvbuff, 0, MAX_LENGTH*sizeof(char));
        }
        //If the user press a key from the console
        if (FD_ISSET(0, &readfds)) //if set to write
        {
            //if enter q, then quit
            fgets (recvbuff, MAX_LENGTH, stdin);
            if ((strlen(recvbuff)>0) && (recvbuff[strlen (recvbuff) - 1] == '\n'))
                recvbuff[strlen (recvbuff) - 1] = '\0';

            if ((strcmp(recvbuff , "q") == 0) || (strcmp(recvbuff , "Q") == 0))
                break;
            else
                printf ("Unrecognized Command.\n");

        }

    }//end while

    close (servsocket);
    return 0;
}
