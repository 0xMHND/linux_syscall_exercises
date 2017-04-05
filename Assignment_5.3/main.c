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
#include <pthread.h>    // for thread

#define MAXLINE 4096
#define h_addr h_addr_list[0]

static char rcv[MAXLINE] = "Hi, Welcome to Perfect Numbers Finder. pls send me the range of numbers you want me to find perfects between.\n";

#define NUM_PROCESSES 32
#define NUM_THREADS 4

static int NUM = NUM_THREADS;
static char mode = 't';

struct info_package{
    uint32_t start;             /*Start number*/
    uint32_t end;               /*Start end*/
    int pid;                    /*Process id*/
};

/*__********************************************************************
 ***********************************************************************
 ***********************************************************************
 *****************    * Functuins Decleration *   **********************
 ***********************************************************************
 ***********************************************************************
 *********************************************************************__*/

/* init pkg */
int init_pkg(struct info_package* pp, unsigned int start, unsigned int end);
/* Parse a line: for parsing the range givin by server*/
char **split_line(char *line, int *cnt);

/* Run processes */
void start_processes( unsigned int start, unsigned int end);
unsigned int RunProcess(void* arg);
/* Run threads */
void start_threads( unsigned int start, unsigned int end);
static void * RunThread(void* arg);
static void * ListenThread(void* arg);

/* Socket */
void client(char *argv[]);

/*find perfect numbers*/
int get_cmd(int *st, int *end, int argc, char *argv[]); // get command line arguments.
int find_perfect(int st, int end); // Assign each process incrementally
int find_perfect2(int st, int step,int end); // Assign each process evenly, it think faster.

void sig_handle(int sig){
   if(sig == SIGQUIT)
      printf("Catched SIGQUIT.. \n");
   if(sig == SIGINT)
      printf("Catched SIGINT.. \n");
   if(sig == SIGUSR1){
      printf("Catched SIG USR1, Exiting.. \n");
      exit(1);
   }
}
/*__********************************************************************
 ***********************************************************************
 ***********************************************************************
 **************************    * MAIN *   ******************************
 ***********************************************************************
 ***********************************************************************
 *********************************************************************__*/

int main(int argc, char *argv[]){

   int start, end;
   printf("Enter 't' for Threads, 'p' for processes: ");
   mode = getchar();
   get_cmd(&start, &end, argc, argv);
   switch(mode){
      case 'p':
	 NUM = NUM_PROCESSES;
	 printf("\n** starting %d processes... **\n\n", NUM);
	 start_processes(start, end);
	 break;
      case 't':
	 NUM = NUM_THREADS;
	 printf("\n** Starting %d THREADS... **\n\n", NUM);
	 start_threads(start, end);
	 break;
      case 'x':
	 client(argv);
      default:
	 printf("smthing is wrong with mode\n");
   }
   printf("\n");
   return 0;
}

/*__********************************************************************
 ***********************************************************************
 ***********************************************************************
 *****************    * Functuins Definitions *   **********************
 ***********************************************************************
 ***********************************************************************
 *********************************************************************__*/

int init_pkg(struct info_package* pp, unsigned int start, unsigned int end)
{
   unsigned int i,j;
   unsigned int step= (end-start)/NUM;
   
   /*set the start and end for each process*/
   for(i=0; i<NUM; i++)
   {
      pp[i].pid = i;
      if(i==0){
	 pp[i].start = start;
      }
      else{
	 pp[i].start = start + (step*i)+1;
      }
      if(i!=(NUM-1)){
	 pp[i].end = start + (step*(i+1));
      }
      else{
	 pp[i].end = end;
      }
   }
   return step;
}

/*__********************************************************************
 **********************    * Processes *   *****************************
 *********************************************************************__*/

void start_processes( unsigned int start, unsigned int end)
{
   struct info_package *pp = (struct info_package*) malloc(sizeof(struct info_package)*NUM_PROCESSES);

   int step = init_pkg(pp,start,end);

   unsigned int i=0;
   for(i=0; i<NUM_PROCESSES; i++){
      switch(fork()){
	 case -1 :
	    printf("Cant fork child #%u\n",i);
	    exit(-1);
	 case 0 :
	    find_perfect2(start+i, step, end);
	    exit(0);
	 default :
	    break;
      }
   }

   unsigned int sum_count=0;
   for(i=0;i<NUM_PROCESSES;i++){
      int k;
      wait(&k);			
   }
   free(pp);
}

unsigned int RunProcess(void* arg){
   
   struct info_package* pp=(struct info_package*)arg;

   find_perfect(pp->start, pp->end);
   
   return 0;
}

/*__********************************************************************
 ************************  * Threads *  ********************************
 *********************************************************************__*/

void start_threads( unsigned int start, unsigned int end)
{
   pthread_t * thr= (pthread_t*) malloc(sizeof(pthread_t)*NUM);
   struct info_package *pkg = (struct info_package*) malloc(sizeof(struct info_package)*NUM);

   int step = init_pkg(pkg,start,end);

   unsigned int i=0;
   for(i=0; i<NUM; i++){
      pthread_create(&thr[i], NULL, RunThread, &pkg[i]);
   }

   void *cnt;
   int sum_count =0;
   for(i=0;i<NUM;i++){
      pthread_join(thr[i],&cnt);
      sum_count = sum_count + (unsigned long long) cnt;
   }
   printf("Thread, total Perfect numbers: %d\n", sum_count);
   free(pkg);
   free(thr);
}

static void * RunThread(void* arg){

   struct info_package* pkg=(struct info_package*)arg;

   unsigned int localCount=0;
   localCount = find_perfect( pkg->start, pkg->end);

   //printf("I'm thread %u, I found %u between %u and  : %u perfect numbers.\n",pkg->pid,pkg->start,pkg->end,localCount);
   
   return (void*)localCount;
}
static void * ListenThread(void* arg){
   int *sock=(int*)arg;

   printf("Thread is reading..\n");
   while(1){
      if(read(*sock, rcv, MAXLINE) == 0){
	 perror("smthing broke");
	 exit(1);
      }
      printf("Server sent: %s", rcv);;
   }
   printf("Thread is DONE..\n");
   return (void*)0;
}
/*__********************************************************************
 **********************  * find_perfect *  *****************************
 *********************************************************************__*/

int find_perfect2(int st, int step,int end)
{
   int i=0;
   int j=st;
   int cnt=0;
   int num = step;
   int sum=0;
  
   
   for( ; j < end+1; j+=NUM_PROCESSES){
      for (i=1; i<((j/2)+1); i++){
   	 if(j%i == 0)
   	    sum = sum + i;
      }

//      printf("I child %d testing %d \n",st, j);
      if(j == sum){
 	 printf("I child %d declare %d as a Perfect #%d\n",st, j, ++cnt);
      }
      sum =0;
   }
   
   //printf("iI'm chld %d, Found %d Perfect numbers\n",st, cnt);

   return cnt;
}

int find_perfect(int st, int end)
{
   int i=0, j=st, cnt=0;
   int num = end;
   int sum=0;
  
   
   for( ;j<num+1; j++){
      for (i=1; i<((j/2)+1); i++){
   	 if(j%i == 0)
   	    sum = sum + i;
      }
      if(j == sum){
 	 printf("%d is Perfect #%d\n", j, ++cnt);
      }
      //else 
	// printf("%d is NOT Perfect because it sum = %d\n", j, sum);
      sum =0;
   }
   
   //printf("iI'm chld %d, Found %d Perfect numbers between %d and %d\n",getpid(),cnt, st, end);
   return cnt;
}

int get_cmd(int *st, int *end, int argc, char *argv[]){
	
   if(argc > 3 || argc == 1){
      printf("Enter cmd line as:{ %s (st) (end)\n", argv[0]);
      exit(EXIT_FAILURE);
   }

   if(argc == 2)
   {
      *st = 1;
      *end = atoi(argv[1]);
      return 0;
   }
   if(atoi(argv[1])<atoi(argv[2]))
   {
      *st = atoi(argv[1]);
      *end = atoi(argv[2]);
      return 0;
   }
   else 
   {
      *st = atoi(argv[2]);
      *end = atoi(argv[1]);
      return 0;
   }

}

void client(char *argv[]){

   /* Signal */
   struct sigaction sa;

  
   sa.sa_handler = sig_handle;
   sa.sa_flags = 0;
   sigemptyset(&sa.sa_mask);
   if (sigaction(SIGINT, &sa, NULL) == -1) {
      perror("sigaction");
      exit(1);
   }
   if (sigaction(SIGQUIT, &sa, NULL) == -1) {
      perror("sigaction");
      exit(1);
   }

   if (sigaction(SIGUSR1, &sa, NULL) == -1) {	
      perror("sigaction");
      exit(1);				     
   }

   /* Variables */
   int sock;
   struct sockaddr_in server;
   struct hostent *hp;
   char sendline[MAXLINE];


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
 
   printf("Connected ..\n");
	
   /* Initial conatct */
   write(sock, rcv, strlen(rcv)+1);
   read(sock, rcv, MAXLINE);
   int cnt;
   char **tokens = split_line(rcv,&cnt);
   int start = atoi(tokens[0]);
   int end = atoi(tokens[1]);
   
   /* Create a thread to read what server send */
   pthread_t thr;
   pthread_create(&thr, NULL, ListenThread, &sock);
      //  start_process(sock);         
 
	 NUM = NUM_THREADS;
	 printf("\n** Starting %d THREADS... **\n\n", NUM);
//	 start_threads(start, end);
 
   pthread_join(thr,0);
   printf("Closing sock.. \n");
   close(sock);

}
#define FSIZE 64
#define DELIM " \n:"
char **split_line(char *line, int *cnt)
{
   int bufsize = FSIZE, pos = 0;
   char **tkns = malloc(bufsize * sizeof(char*));
   char *tkn;

   if (!tkns) {
      fprintf(stderr, "lsh: allocation error\n");
      exit(EXIT_FAILURE);
   }

   tkn = strtok(line, DELIM);
   while (tkn != NULL) {
      tkns[pos] = tkn;
      pos++;

      if (pos >= bufsize) {
	 bufsize += FSIZE;
	 tkns = realloc(tkns, bufsize * sizeof(char*));
	 if (!tkns) {
	    fprintf(stderr, "lsh: allocation error\n");
	    exit(EXIT_FAILURE);
	 }
      }

      tkn = strtok(NULL, DELIM);
   }
   tkns[pos] = NULL;
   *cnt = pos;
   return tkns;
}
