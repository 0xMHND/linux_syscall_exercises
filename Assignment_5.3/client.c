#define _POSIX_C_SOURCE 200908L

#include <sys/types.h>  // basic system data types
#include <sys/socket.h> // basic socket definitions
#include <sys/time.h>   // timeval{} for select()
#include <time.h>       // timespec{} for pselect()
#include <netinet/in.h> // sockaddr_in{} and other Internet defns
#include <arpa/inet.h>  // inet(3) functions
#include <errno.h>
#include <fcntl.h>      // for nonblocking
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>   // for S_xxx file mode constants
#include <sys/uio.h>    // for iovec{} and readv/writev
#include <unistd.h>
#include <sys/wait.h>
#include <sys/select.h>

#define LISTENQ 1024
#define MAXLINE 4096
#define MAXSOCKADDR 128
#define BUFFSIZE 4096

#define SERV_PORT 9878
#define SERV_PORT_STR "9878"


int main(int argc, char **argv)
{
  int i;
  int sock;
  struct sockaddr_in servaddr;
  memset(&servaddr, '\0', sizeof(servaddr));
  
  struct hostent *hp;
  char * DATA = "Hi, I'm Client";
  char sendline[MAXLINE];
  char recvline[MAXLINE];
  memset(&recvline, '\0', MAXLINE);

  printf("Client started...\n");

  sock = socket(AF_INET, SOCK_STREAM, 0);
  if(sock<0){
     perror("socket failed\n");
     exit(1);
  }
  
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(5000);
  hp = gethostbyname(argv[1]);
  if(hp == 0){
     perror("gethostbyname failed\n");
     exit(1);
  }
  memcpy(&servaddr.sin_addr, hp->h_addr_list[0], hp->h_length);

  //inet_pton(AF_INET, argv[1], &servaddr.sin_addr);
  printf("Before connect\n"); 
  
  if(connect(sock, (struct sockaddr *)&servaddr, sizeof(servaddr))<0)
  {
     perror("Connect failed");
     close(sock);
     exit(1);
  }
  printf("Before send\n"); 
  if(send(sock, DATA, sizeof(DATA),0)<0){
     perror("Send FAiled\n");
     close(sock);
     exit(1);
  }

  printf("Sent : %s\n", DATA);
  close(sock);

  return 0;
}
