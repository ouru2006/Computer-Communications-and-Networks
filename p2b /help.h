#define DAT 5
#define ACK 1
#define SYN 2
#define FIN 3
#define RST 4
#define BUFFER_SIZE 10240
#define PACKAGE_SIZE 1024
#define PAYLOAD_LENGTH 900
const char* type[6]={"error","ACK","SYN","FIN","RST","DAT"};
struct header {
  char* Magic;
  int Type_i;
  int Seqno;
  int Ackno;
  int Paylen;
  int Winsize;
  char payload[PAYLOAD_LENGTH];
};
struct summery_message_s{
  int total_bytes;
  int unique_bytes;
  int total_packets;
  int unique_packets;
  int SYN_s;
  int FIN_s;
  int RST_s;
  int ACK_r;
  int RST_r;
  float time_dn;
};

struct summery_message_r{
  int total_bytes;
  int unique_bytes;
  int total_packets;
  int unique_packets;
  int SYN_r;
  int FIN_r;
  int RST_r;
  int ACK_s;
  int RST_s;
  float time_dn;
};
void log_message(char event,struct sockaddr_in source,struct sockaddr_in destination,int type_no, int SorA, int LorW){
  char time_string[9];
  bzero(time_string, 9);
  struct timeval tv;
  gettimeofday(&tv, NULL);
  strncpy(time_string,ctime(&tv.tv_sec)+11,8);
  printf("%s.%06ld %c %s:%d ",time_string,tv.tv_usec,event,inet_ntoa(source.sin_addr),ntohs(source.sin_port));
  printf("%s:%d %s %d %d\n" ,inet_ntoa(destination.sin_addr),ntohs(destination.sin_port), type[type_no],SorA,LorW);
}
//struct
void print_log_s(struct summery_message_s logs){
  printf("total data bytes sent: %i\n", logs.total_bytes);
  printf("unique data bytes sent: %i\n", logs.unique_bytes);
  printf("total data packets sent: %i\n", logs.total_packets);
  printf("unique data packets sent: %i\n", logs.unique_packets);
  printf("SYN packets sent: %i\n", logs.SYN_s);
  printf("FIN packets sent: %i\n", logs.FIN_s);
  printf("RST packets sent: %i\n", logs.RST_s);
  printf("ACK packets received: %i\n", logs.ACK_r);
  printf("RST packets received: %i\n", logs.RST_r);
  printf("total time duration (second): %.3f\n", logs.time_dn);
}

void print_log_r(struct summery_message_r logs){
  printf("total data bytes received: %i\n", logs.total_bytes);
  printf("unique data bytes received: %i\n", logs.unique_bytes);
  printf("total data packets received: %i\n", logs.total_packets);
  printf("unique data packets received: %i\n", logs.unique_packets);
  printf("SYN packets received: %i\n", logs.SYN_r);
  printf("FIN packets received: %i\n", logs.FIN_r);
  printf("RST packets received: %i\n", logs.RST_r);
  printf("ACK packets send: %i\n", logs.ACK_s);
  printf("RST packets send: %i\n", logs.RST_s);
  printf("total time duration (second): %.3f\n", logs.time_dn);
}
