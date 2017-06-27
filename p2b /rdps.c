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
struct summery_message_s logs;
void init_packet(){
  packet.Magic="CSC361";
  packet.Type_i = SYN;
  srand(time(NULL));
  packet.Seqno =  rand()%100;
  packet.Ackno = 0;
  packet.Paylen = 0;
  packet.Winsize = BUFFER_SIZE;

}
void init_log(){
  logs.total_bytes=0;
  logs.unique_bytes=0;
  logs.total_packets=0;
  logs.unique_packets=0;
  logs.SYN_s=0;
  logs.FIN_s=0;
  logs.RST_s=0;
  logs.ACK_r=0;
  logs.RST_r=0;
  logs.time_dn=0;
}

void RDP_Send(int socket_fd, struct sockaddr_in receiver){
  char send_buffer[PACKAGE_SIZE];
  bzero(send_buffer,PACKAGE_SIZE);
  if(packet.Type_i==DAT){

    memcpy(send_buffer,packet.payload,packet.Paylen);

    logs.total_packets++;
    logs.total_bytes+=packet.Paylen;
  }else{
    packet.Paylen = strlen(packet.payload);
  }
  send_buffer[PAYLOAD_LENGTH]=packet.Type_i;
  send_buffer[PAYLOAD_LENGTH+1]='\0';
  //sprintf(send_buffer+PAYLOAD_LENGTH,"%d",packet.Type_i);
  memcpy(send_buffer+PAYLOAD_LENGTH+1,packet.Magic,strlen(packet.Magic));
  sprintf(send_buffer+PAYLOAD_LENGTH+10,"%d",packet.Seqno);
  sprintf(send_buffer+PAYLOAD_LENGTH+20,"%d",packet.Ackno);
  sprintf(send_buffer+PAYLOAD_LENGTH+30,"%d",packet.Paylen);
  sprintf(send_buffer+PAYLOAD_LENGTH+40,"%d",packet.Winsize);
  if(sendto(socket_fd, send_buffer, PACKAGE_SIZE, 0, (struct sockaddr *)&receiver, sizeof(receiver))==-1){
    error("error in sendto()");
    close(socket_fd);
    exit(1);
  }
  //printf("%f\n", send_buffer);
  if(packet.Type_i==SYN)logs.SYN_s++;
  else if(packet.Type_i==FIN)logs.FIN_s++;
  else if(packet.Type_i==RST)logs.RST_s++;
  else if(packet.Type_i==FIN)logs.FIN_s++;

}
/*
*/
int main(int argc, char **argv) {
  struct timeval start, end;

  int window=BUFFER_SIZE;
  int socket_fd;
  struct sockaddr_in sender, receiver;
  socklen_t add_len = sizeof(struct sockaddr_in);
  char receive_data[BUFFER_SIZE];
  int retransmit=0;//timer
//  struct rdp_connection sender_con;
  char* filename;
  char event;
  //check command line arguments
  if (argc < 6) {
     fprintf(stderr,"usage: %s <sender_ip><sender_port> <receiver_ip> <receiver_port><filename>\n", argv[0]);
     exit(0);
   }
   //get filename
   filename = argv[5];

   //socket: create the socket
   if ((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
   {
     error("socket()");
   }

   // Sender address.
   memset(&sender, 0, sizeof(sender));
   sender.sin_family = AF_INET;
   sender.sin_addr.s_addr = inet_addr(argv[1]);
   sender.sin_port = htons(atoi(argv[2]));

   // Receiver address.
   memset(&receiver, 0, sizeof(receiver));
   receiver.sin_family = AF_INET;
   receiver.sin_addr.s_addr = inet_addr(argv[3]);
   receiver.sin_port = htons(atoi(argv[4]));

   int optval = 1;
   setsockopt(socket_fd,SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR), &optval, sizeof(optval));

   if (bind(socket_fd,(struct sockaddr *)&sender, sizeof(sender)) == -1)
   {
       error("bind()");
   }

  // Establish connection with receiver.

  //init packet
  init_packet();

  //init log
  init_log();
  int random_seq=packet.Seqno;
  //read file
  FILE *input;
  input= fopen(filename, "r");
  fseek(input, 0L , SEEK_END);
  int file_bytes;
  long int fsize = ftell(input);
  char *file=malloc(fsize + 1);
  fseek(input, 0L, SEEK_SET);
  file_bytes=fread(file, sizeof(char), fsize, input);

  gettimeofday(&start, NULL);
  event = 's';
  while(1){
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 10000;
    if (packet.Type_i == SYN || packet.Type_i == FIN){
      RDP_Send(socket_fd, receiver);
      log_message(event,sender,receiver,packet.Type_i,packet.Seqno,packet.Paylen);
      timeout.tv_sec = 0;
      timeout.tv_usec = 100000;
    }
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
      //printf("aaaaaaaaaaa\n");
      int bytes_read = recvfrom(socket_fd,receive_data,BUFFER_SIZE,0,(struct sockaddr *)&receiver, &add_len);
      packet.Winsize=atoi(receive_data+10);
      //printf("%d  %d  %d  %d\n",packet.Winsize,atoi(receive_data+1),packet.Seqno,packet.Ackno);
      retransmit=0;
      if(receive_data[0]==ACK){
        logs.ACK_r++;
        if(packet.Ackno < atoi(receive_data+1)){
          packet.Ackno = atoi(receive_data+1);
          event = 'r';
          log_message(event,receiver,sender,receive_data[0],packet.Ackno,packet.Winsize);
          if(packet.Type_i==FIN){
            break;
          }
          //printf("%d %d\n",packet.Seqno,atoi(receive_data+1));
          if(packet.Seqno <= atoi(receive_data+1)){
            packet.Seqno = atoi(receive_data+1);
            event = 's';
            if( (packet.Seqno-random_seq-1 < file_bytes)){
              //printf("////////////////////////////////////DAT\n" );
              packet.Type_i=DAT;
             }else {
              packet.Type_i=FIN;
              continue;
             }
          }else{
            //printf("22222222222\n");
            event = 'S';
            packet.Type_i=ACK;
            continue;
          }
        }else if(packet.Ackno >= atoi(receive_data+1)){
          event = 'R';
          log_message(event,receiver,sender,receive_data[0],atoi(receive_data+1),packet.Winsize);
          if(packet.Type_i==DAT){
            packet.Type_i = ACK;
          }
          continue;
        }
      }else if(receive_data[0]==RST){
        logs.RST_r++;
        //init packet
        init_packet();
        //init log
        init_log();
        event = 's';
        packet.Type_i=SYN;
        continue;
      }
    }
    if (packet.Type_i == DAT ){
      window=packet.Winsize;
      int temp=packet.Seqno;
      if(event == 'S'){
        window = packet.Winsize;
        packet.Seqno=packet.Ackno;
      }
      while (window>=PACKAGE_SIZE && packet.Seqno-random_seq-1 < file_bytes) {
        /* code */
        memcpy(packet.payload,file+packet.Seqno-random_seq-1,PAYLOAD_LENGTH);
        if( file_bytes-(packet.Seqno-random_seq-1) >=PAYLOAD_LENGTH)
          packet.Paylen = sizeof(packet.payload);
        else
          packet.Paylen = file_bytes-(packet.Seqno-random_seq-1);
        if(event == 's'||packet.Seqno >= temp){
          event='s';
          logs.unique_bytes += packet.Paylen;
          logs.unique_packets++;
        }
        RDP_Send(socket_fd,receiver);
        window -= packet.Paylen;
        log_message(event,sender,receiver,packet.Type_i,packet.Seqno,packet.Paylen);
        packet.Seqno += packet.Paylen;
        bzero(packet.payload, PAYLOAD_LENGTH);
      }
      packet.Type_i=ACK;
      continue;
    }

    if(select_id== 0)
    {
      //printf("////////////////////////////////////gap\n" );
      if(packet.Type_i==FIN&&retransmit==3){
        printf("FIN no reply\n");
        break;
      }
      if(retransmit<10){
        event = 'S';
        if(packet.Type_i==ACK){
          packet.Type_i = DAT;
        }
        retransmit++;
        continue;
      }else if(retransmit==10){

        packet.Type_i=RST;
        event = 's';
        RDP_Send(socket_fd, receiver);
        log_message(event,sender,receiver,packet.Type_i,packet.Seqno,packet.Paylen);
        retransmit++;
        continue;
      }else{
        event = 's';
        packet.Type_i = FIN;
        RDP_Send(socket_fd, receiver);
        log_message(event,sender,receiver,packet.Type_i,packet.Seqno,packet.Paylen);
        printf("timeout\n");
        break;
      }
    }
  }
  gettimeofday(&end, NULL);
  logs.time_dn=(float)((end.tv_sec) * 1e6 + end.tv_usec-((start.tv_sec) * 1e6 + start.tv_usec) )/1e6 ;
  //printf("%s\n",type[0]);
  // printf("%s:%d %s:%d\n",inet_ntoa(sender.sin_addr),ntohs(sender.sin_port),inet_ntoa(receiver.sin_addr),ntohs(receiver.sin_port));
  print_log_s(logs);
  fclose(input);
  free(file);
  close(socket_fd);
  return 0;
}
