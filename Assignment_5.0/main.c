#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <pthread.h>
#include <stdint.h> //uint32_t
#include <unistd.h> // fork()
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#define NUM_PROCESSES 32

struct info_package{
    uint32_t start;             /*Start number*/
    uint32_t end;               /*Start end*/
    int pid;                    /*Process id*/
};


/*init and run processes*/
void init_pp(struct info_package* pp, unsigned int start, unsigned int end);
void start_processes( unsigned int start, unsigned int end);
unsigned int RunProcess(void* arg);

/*find perfect numbers*/
int get_cmd(int *st, int *end, int argc, char *argv[]);
int find_perfect(int st, int end);


int main(int argc, char *argv[]){

   int start, end;
   get_cmd(&start, &end, argc, argv);
   start_processes(start, end);

   return 0;
}

void init_pp(struct info_package* pp, unsigned int start, unsigned int end)
{
   unsigned int i,j;
   unsigned int step= (end-start)/NUM_PROCESSES;
   
   /*set the start and end for each process*/
   for(i=0; i<NUM_PROCESSES; i++)
   {
      pp[i].pid = i;
      if(i==0){
	 pp[i].start = start;
      }
      else{
	 pp[i].start = start + (step*i)+1;
      }
      if(i!=(NUM_PROCESSES-1)){
	 pp[i].end = start + (step*(i+1));
      }
      else{
	 pp[i].end = end;
      }
   }

}
void start_processes( unsigned int start, unsigned int end)
{
   struct info_package *pp = (struct info_package*) malloc(sizeof(struct info_package)*NUM_PROCESSES);
/*
   struct sigaction s, t;
   sigset_t blockSet, prevMask;
   sigemptyset(&blockSet);
   sigaddset(&blockSet, SIGINT);
   sigaddset(&blockSet, SIGQUIT);
   if(sigprocmask(SIG_BLOCK, &blockSet, &prevMask) == -1)
      exit(-1);
  */ 
   init_pp(pp,start,end);

   unsigned int i=0;
   for(i=0; i<NUM_PROCESSES; i++){
    //  sigaddset(&blockSet, SIGCHLD);
      //if(sigprocmask(SIG_BLOCK, &blockSet, &prevMask) == -1)
      	// exit(-1);
      switch(fork()){
	 case -1 :
	    printf("Cant fork child #%u\n",i);
	    exit(-1);
	 case 0 :
      	  //  if(sigprocmask(SIG_BLOCK, &prevMask, NULL) == -1)
	    //   exit(-1);
	    RunProcess(&pp[i]);
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
   
   printf("iI'm chld %d, Found %d Perfect numbers between %d and %d\n",getpid(),cnt, st, end);
   return cnt;
}

int get_cmd(int *st, int *end, int argc, char *argv[]){
	
   if(argc > 3 || argc == 1){
      printf("Enter cmd line as : name start_number end_number\n");
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
