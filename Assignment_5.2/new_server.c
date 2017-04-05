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



int main(int argc,char *argv[])
{

   /* Variables */
   int sock;
   struct sockaddr_in server;
   int mysock;
   char buff[1024];
   int rval;


   /* Create socket */
   sock = socket(AF_INET, SOCK_STREAM, 0);
   if(sock<0){
      perror("Failed to creat socket");
      exit(1);
   }

   server.sin_family = AF_INET;
   server.sin_addr.s_addr = INADDR_ANY;
   server.sin_port = htons(5000);

   /* Bind */
  if(bind(sock, (struct sockaddr *)&server, sizeof(server))){
     perror("bind fail");
     exit(1);
  }

  /* Listen */
  listen(sock, 5);
  printf("Listening.. \n");
  /* Accept */
  do{
     mysock = accept(sock, (struct sockaddr *)0 ,0);
     if(mysock == 0)
	perror("accept failed");

     else
     {
	printf("Client connected\n");
	memset(buff, 0, sizeof(buff));

	if((rval = recv(mysock, buff, sizeof(buff), 0)) < 0){
	   perror("READING STREAM FAILED");
	}
	else if(rval == 0){
	   printf("Ending connection\n");
	}
	else
	   printf("MSG: %s\n", buff);

	printf("Got the message (rval = %d)\n",rval);
	close(mysock);
     }
  }while(1);

   return 0;
}
