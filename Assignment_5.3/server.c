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

int main(int argc, char **argv)
{
   /* Variables */ 
  int i;
  int mysockfd;
  int sock;
  int port;
  int n;

  char buf[MAXLINE];

 
  struct sockaddr_in servaddr;
  memset(&servaddr, 0, sizeof(servaddr));

  /* Create socket */
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if(sock<0){
     perror("Failed to create socket\n");
     exit(1);
  }

  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = INADDR_ANY;
  servaddr.sin_port = htons(5000);

  /* Bind it */
  if(bind(sock, (struct sockaddr *)&servaddr, sizeof(servaddr)))
  {
     perror("bind failed");
     exit(1);
  }

  /* Listen */
  listen(sock, 5);

  printf("Server is Listening ...\n");
  /* Accept */
  do {
    
     mysockfd = accept(sock, (struct sockaddr *)0, 0);
     if(mysockfd == -1)
	perror("Accept failed");

     else{	
	printf("Connection ACCEPTED\n");
      	if((n = recv(mysockfd, buf, MAXLINE,0)) < 0){     
	   perror("READING STREAM MESSAGE ERROR\n");
	   close(mysockfd);
	   break;
	}
	else if(n == 0)
	   perror("Ending connectoin\n");
	else
	   printf("MSG: %s\n",buf );


     	printf("Got the message (rval = %d)\n", n);
	close(mysockfd);
     }
  }while(1);
  
  return 0;
}
