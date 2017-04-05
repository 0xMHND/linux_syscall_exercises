
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


#define DATA "Hi Im a new client"
#define h_addr h_addr_list[0]
int main(int argc,char *argv[])
{

   /* Variables */
   int sock;
   struct sockaddr_in server;
   struct hostent *hp;
   char buff[1024];


   /* Create socket */
   sock = socket(AF_INET, SOCK_STREAM, 0);
   if(sock<0){
      perror("Failed to creat socket");
      exit(1);
   }

   server.sin_family = AF_INET;
   
   if( (hp = gethostbyname(argv[1])) == 0 ){
      perror("gethostbyname failed");
     close(sock);
      exit(1);
   }
   memcpy(&server.sin_addr, hp->h_addr, hp->h_length);
   
   server.sin_port = htons(5000);

   /* connect */
  if(connect(sock, (struct sockaddr *)&server, sizeof(server))<0){
     perror("connnection failed");
     close(sock);
     exit(1);
  }

  if(send(sock, DATA, sizeof(DATA),0) <0){
     perror("send failed");
     close(sock);
     exit(1);
  }
  printf("sent: %s\n", DATA);
  close(sock);

  return 0;
}
