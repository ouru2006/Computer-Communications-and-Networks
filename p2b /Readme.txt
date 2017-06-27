P2 Readme
Ru Ou
V00835123

This project includes a Makefile, rdps.c, rdpr.c,help.h, Readme.txt.

1. Go to the file directory and type 'make' on commend line to run the make file.

2. Type './rdpr 10.10.1.100 8000 receiver.dat' in a terminal to run the receiver side.

3. Type './rdps 192.168.1.100 8000 10.10.1.100 8000 sent.dat' in another terminal to run the sender side.

4. Type 'make clean' to remove rdps, rdpr and receiver.dat

My design:
1. How do you design and implement your RDP header and header fields? Do you use any additional header fields?

I create a header structure and define 5 states as integer in help.h file shown below.
#define DAT 0
#define ACK 1
#define SYN 2
#define FIN 3
#define RST 4
struct header {
  char* Magic;
  int Type_i;
  int Seqno;
  int Ackno;
  int Paylen;
  int Winsize;
  char payload[BUFFER_SIZE];
};
In my design, I set state type as int type and define state as integer. For example, I can use packet.Type_i=SYN to represent the SYN state.
In my header, I also set a Payload[BUFFER_SIZE] to store data.

2. How do you design and implement the connection management using SYN, FIN and RST packets? How to choose the initial sequence number?

It is a 2 way handshake to implement the connection management.
I initial the packet type as SYN before while loop in sender, and send SYN to receiver. If receiver received SYN, then send ACK to sender.
If sender receives ACK, then connect establishes. If sender don’t receives ACK and timeout, sender will retransmit SYN again. If sender does not receive ACK after retransmit some times, then send RST. If still not receive anything, then timeout and close sender.


If sender want to release connection, then set packet type as FIN and send FIN. If receive ACK, then close sender.

Randomly choose the initial sequence number.

3. How do you design and implement the flow control using window size? How to choose the initial window size and adjust the size?
How to read and write the file?
How to manage the buffer at the sender and receiver side, respectively?

Only receiver need window size to control the receive_data size, the receive data size cannot bigger than window size. If there is no enough window size, sender will not be allowed to send any more data.
I set the initial window size as BUFFER_SIZE and adjust it by minus data byte of receive_data size and add write file size.

this is my read file function:
int read_file(char* filename)
{
  FILE *input;
  input= fopen(filename, "r");
  if(input==NULL){
    return (-1);
  }else{
    fseek(input, 0L , SEEK_END);
    int bytes_read;
    long int fsize = ftell(input);
    char *file=malloc(fsize + 1);
    fseek(input, 0L, SEEK_SET);
    bytes_read=fread(file, sizeof(char), fsize, input);
    fclose(input);
    free(file);
    return 0;
  }
}

This is my write file function:
void write_file(char* filename,char recieve_data[],int file_bytes){
  FILE *file;
  file = fopen(filename, "a");
  fwrite (recieve_data, sizeof(char), file_bytes, file);
  fclose(file);
}

At the receiver side, the buffer size is depends on window size. In my design, if window size less than 4*packet_size, then write buffer to file and reset window size as max BUFFER_SIZE, which is 10240. And if receive FIN, then write buffer to file.
At the sender side, get the window size from receive_buffer and send multiple packets, which total data size less than window size.


4. How do you design and implement the error detection, notification and recovery?
How to use timer?
How many timers do you use?
How to respond to the events at the sender and receiver side, respectively?
How to ensure reliable data transfer?

Use retransmit to implement the error detection, notification and recovery.
Use select to wait data receiving, if timeout, then retransmit segment and start timer. And set retransmit++ to record the retransmission time. After retransmit a segment some times and still not get ACK, then send RST and start timer again. If still no ACK, then exit.
I used one timer.

sender side:
event:get data from read file
	create segment with sequence number Seqno
	send segment to receiver
	Seqno+=length(data)
	start timer
	break;
event:timer timeout
	if(retransmit<2)
		retransmit not-yet-acknowledge segment
		retransmit++;
		start timer
		break;
	else if(retransmit==2)
		send RST
		retransmit++;
		start timer
		break
	else
		timeout;
		break;
event:ACK received, with newAckno
	retransmit=0
	if (newAckno > Ackno)
		Ackno = newAckno
	else
		there are currently any not-yet=acknowledged segments
		start timer
	break;
event:RST received
	retransmit=0
  init all things
	send SYN
	start timer
	break;
event: all data sent successfully
	send FIN
	start timer
	break;

receiver side:
event: SYN received
	init packet
	send ACK with newAckno
	break;
event: FIN received
	send ACK with newAckno
	close();
	break;
event: RST received
	send ACK with newAckno
	break;
event: DAT received
	read data
	adjust window size
	send ACK with newAckno
	break;
event: timeout
  retransmit last packet or close
  break;

To ensure reliable data transfer, we use the ackno to check if any packet lost. If lost, resubmit data from the received ackno, like set seqno=ackno

5. Any additional design and implementation considerations you want to get feedback from your lab instructor?
I did something for RST, if send RST, then both side will initialize all content in packet and clean the write file. Then sender will send SYN to establish connection again.
