#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/select.h>//use select() for multiplexing
#include <sys/fcntl.h> // for non-blocking
#include <time.h>
#include "help.h"


struct header packet;
struct summery_message_r logs;
void init_packet(){
  packet.Type_i = SYN;
  packet.Seqno =  0;
  packet.Ackno = 0;
  packet.Paylen = 0;
  packet.Winsize = BUFFER_SIZE;

}
void init_log(){
  logs.total_bytes=0;
  logs.unique_bytes=0;
  logs.total_packets=0;
  logs.unique_packets=0;
  logs.SYN_r=0;
  logs.FIN_r=0;
  logs.RST_r=0;
  logs.ACK_s=0;
  logs.RST_s=0;
  logs.time_dn=0;
}
void init_file(char* filename){
  FILE *file;
  if((file = fopen(filename,"w"))==NULL)
    printf("init_file error" );
  fclose(file);
}
void write_file(char* filename,char file_buffer[]){

  FILE *file;
  file = fopen(filename, "a");
  fwrite (file_buffer, sizeof(char), BUFFER_SIZE-packet.Winsize, file);
  fclose(file);
  bzero(file_buffer, BUFFER_SIZE);
  packet.Winsize=BUFFER_SIZE;
}
void RDP_Send(int socket_fd, struct sockaddr_in sender)
{
  char send_buffer[PACKAGE_SIZE];
  send_buffer[0]=packet.Type_i;
  //printf("%d\n",packet.Ackno );
  sprintf(send_buffer+1,"%d",packet.Ackno);
  sprintf(send_buffer+10,"%d",packet.Winsize);
  if(sendto(socket_fd, send_buffer, sizeof(send_buffer), 0, (struct sockaddr *)&sender, sizeof(sender))==-1){
    error("error in sendto()");
    close(socket_fd);
    exit(1);
  }
  if(packet.Type_i==ACK)
    logs.ACK_s++;
  else if(packet.Type_i==RST)
    logs.RST_s++;
}

int main(int argc, char **argv) {
  int file_offset=0;
  int retransmit=30;
  char event;
  struct timeval start, end;
  gettimeofday(&start, NULL);

  int socket_fd;
  struct sockaddr_in receiver,sender;
  socklen_t add_len = sizeof(struct sockaddr_in);
  char* filename;
  struct timeval timeout;
  char file_buffer[BUFFER_SIZE];
  //char send_buffer[BUFFER_SIZE];
  timeout.tv_sec = 10;
  timeout.tv_usec = 0;

  if (argc < 4) {
     fprintf(stderr,"usage: %s <receiver_ip> <receiver_port> <filename>\n", argv[0]);
     exit(0);
   }
   filename = argv[3];

   //socket: create the socket
   if ((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
   {
     error("socket()");
   }

   memset(&sender, 0, sizeof(sender));
   // Receiver address.
   memset(&receiver, 0, sizeof(receiver));
   receiver.sin_family = AF_INET;
   receiver.sin_addr.s_addr = inet_addr(argv[1]);
   receiver.sin_port = htons(atoi(argv[2]));

   int optval = 1;
   setsockopt(socket_fd,SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR), &optval, sizeof(optval));
   if (bind(socket_fd,(struct sockaddr *)&receiver, sizeof(receiver)) == -1)
   {
       error("bind()");
   }

   //init packet
   init_packet();

   while(1){
     fd_set readfds;
     FD_ZERO(&readfds);
     FD_SET(socket_fd, &readfds);
     int select_id=select(socket_fd + 1, &readfds, 0, 0, &timeout);
     if (select_id==-1)
     {
       perror("select"); // error occurred in select()
       break;
     }

     if (FD_ISSET(socket_fd, &readfds)){
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;
        retransmit=0;
        char receive_data[BUFFER_SIZE];
        bzero(receive_data, BUFFER_SIZE);
        int bytes_read = recvfrom(socket_fd,receive_data,packet.Winsize,0,(struct sockaddr *)&sender, &add_len);

        if(receive_data[PAYLOAD_LENGTH]==SYN||atoi(receive_data+PAYLOAD_LENGTH+10)==packet.Ackno){
          event = 'r';
          packet.Seqno=atoi(receive_data+PAYLOAD_LENGTH+10);
          packet.Ackno=atoi(receive_data+PAYLOAD_LENGTH+10);
          packet.Paylen=atoi(receive_data+PAYLOAD_LENGTH+30);
          log_message(event,sender,receiver,receive_data[PAYLOAD_LENGTH],packet.Seqno,packet.Paylen);
        }else if(atoi(receive_data+PAYLOAD_LENGTH+10)<packet.Ackno){
          event = 'R';
          log_message(event,sender,receiver,receive_data[PAYLOAD_LENGTH],packet.Seqno,packet.Paylen);
        }else{
          event = 'r';
          log_message(event,sender,receiver,receive_data[PAYLOAD_LENGTH],packet.Seqno,packet.Paylen);
          event = 's';
        }
        //printf("%d\n", packet.Winsize);
        switch (receive_data[PAYLOAD_LENGTH]){
          case SYN:
          //init log
            init_log();
            init_file(filename);
            if(event=='r'){
              packet.Winsize=atoi(receive_data+PAYLOAD_LENGTH+40);
              logs.SYN_r++;
              packet.Ackno++;
              event = 's';
            }else if(event=='R'){
              event = 'S';
            }
            //strncpy(packet.Magic,receive_data+PAYLOAD_LENGTH+1,6);
            packet.Type_i=ACK;
            RDP_Send(socket_fd,sender);
            log_message(event,receiver,sender,packet.Type_i,packet.Ackno,packet.Winsize);
            break;
          case FIN:
            if(event=='r'){
              logs.FIN_r++;
              packet.Ackno++;
              event = 's';
            }else if(event=='R'){
              event = 'S';
            }
            packet.Type_i=ACK;
            RDP_Send(socket_fd, sender);
            log_message(event,receiver,sender,packet.Type_i,packet.Ackno,packet.Winsize);
            write_file(filename,file_buffer);
            file_offset=0;
            packet.Type_i=FIN;
            break;
          case RST:
            if(event=='r'){
              logs.RST_r++;
              packet.Ackno++;
              event = 's';
            }else if(event=='R'){
              event = 'S';
            }

            packet.Type_i=RST;
            RDP_Send(socket_fd, sender);
            log_message(event,receiver,sender,packet.Type_i,packet.Ackno,packet.Winsize);
            //init log
            init_log();
            init_packet();
            init_file(filename);
            break;
          case DAT:
            if(event=='r'){
              logs.unique_bytes+=packet.Paylen;
              logs.unique_packets++;
              packet.Ackno+=packet.Paylen;
              memcpy(file_buffer+file_offset,receive_data,packet.Paylen);
              packet.Winsize -= packet.Paylen;
              file_offset += packet.Paylen;
              event = 's';
            }else if(event=='R'){
              event = 'S';
            }
            if(packet.Winsize<4*PACKAGE_SIZE){
              write_file(filename,file_buffer);
              file_offset=0;
            }//flow control
            logs.total_packets++;
            logs.total_bytes += packet.Paylen;
            packet.Type_i=ACK;
            RDP_Send(socket_fd, sender);
            log_message(event,receiver,sender,packet.Type_i,packet.Ackno,packet.Winsize);
            break;
        }

     }
     if(packet.Type_i==FIN){
       timeout.tv_sec = 0;
       timeout.tv_usec = 300000;
     }

     if(select_id== 0)
     {
       //timeout
         //continue;
         if(packet.Type_i==FIN){
           break;
         }
         else if(retransmit<10){
           event = 'S';
           RDP_Send(socket_fd, receiver);
           log_message(event,receiver,sender,packet.Type_i,packet.Ackno,packet.Winsize);
           retransmit++;
           continue;
         }else if(retransmit==10){
           packet.Type_i=RST;
           event = 's';
           RDP_Send(socket_fd, receiver);
           log_message(event,receiver,sender,packet.Type_i,packet.Ackno,packet.Winsize);
           retransmit++;

           continue;
         }else{
           //continue;
           printf("timeout\n");
           break;
         }

     }

   }
   gettimeofday(&end, NULL);
   logs.time_dn=(double)((end.tv_sec) * 1e6 + end.tv_usec-((start.tv_sec) * 1e6 + start.tv_usec) ) /1e6;
    //printf("%s\n",type[0]);
    print_log_r(logs);
    close(socket_fd);
    return 0;
}
