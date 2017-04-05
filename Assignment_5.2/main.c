#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <pthread.h> // for threads
#include <stdint.h> //uint32_t
#include <unistd.h> // fork()
#include <signal.h> // For signals
#include <sys/types.h>
#include <sys/wait.h>

#define GREET_MSG "Hi Im a new client"
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

/* Run processes */
void start_processes( unsigned int start, unsigned int end);
unsigned int RunProcess(void* arg);
/* Run threads */
void start_threads( unsigned int start, unsigned int end);
static void * RunThread(void* arg);

/*find perfect numbers*/
int get_cmd(int *st, int *end, int argc, char *argv[]); // get command line arguments.
int find_perfect(int st, int end); // Assign each process incrementally
int find_perfect2(int st, int step,int end); // Assign each process evenly, it think faster.

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
