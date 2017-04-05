#define _POSIX_C_SOURCE 200908L
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <math.h>
#include <utime.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <glob.h>
#include <ctype.h>
#include <signal.h>

#define NUM_THREADS 1
#define NUM_PROCESSES 45

static char mode = 'p';
static unsigned int LIMIT= UINT32_MAX;

//typedef uint32_t word_t;
enum { BITS_PER_WORD = sizeof(char)*8 };
#define WORD_OFFSET(b) ((b) / BITS_PER_WORD)
#define BIT_OFFSET(b)  ((b) % BITS_PER_WORD)
static char *address;   /*Global bitmap address*/
unsigned long long sum_count;
/*__*********************************************************************
 ************************************************************************
 ************************************************************************
 ************************    * STRUCTS *   ******************************
 ************************************************************************
 ************************************************************************
 ********************************************************************__*/

struct info_package{
    uint32_t start;             /*Start location in the bitmap*/
    uint32_t end;               /*Start location in the bitmap*/
    struct node *primes_que;    /*Primes used to generate new primes.*/
    int pid;                    /*Process id*/
};

struct node{
    uint32_t prime;         /*Current node prime*/
    struct node *next;      /*Next prime*/

};


/*__********************************************************************
 ************************************************************************
 ************************************************************************
 ******************    * Functuins Decleration *   **********************
 ************************************************************************
 ************************************************************************
 ********************************************************************__*/


/*create bitmap and store the map address in "address"*/
void create_bitmap();

/*Access the bitmap*/
int check_Prime(unsigned int n);
void set_NotPrime(unsigned int n);
void set_NotHappy(unsigned int n);

/*get the seed primes. primes less than sqrt(limit)*/
void SEEDING_Primes(unsigned int max);
unsigned int set_SEEDING_HappyPrimes(unsigned int max);
/*store the seeding primes into each process que*/
void create_primes_que(struct info_package* pp, unsigned int start, unsigned int end);

/*Start the processes work.*/
void start_processes(unsigned int start, unsigned int end);
unsigned int RunProcess(void* arg);

/*Start the threads work.*/
void start_threads( unsigned int start, unsigned int end);
static void * RunThread(void* arg);

void find_happy_prime(int *nprime,int cnt, uint32_t limit);

/*Find primes*/
void find_primes(uint32_t limit, int *cnt, int *nprime);
void p_print(int *nprime,int cnt, uint32_t limit);
void print_primes(unsigned int limit);

/*Find Happy Numbers*/
uint32_t check_happy(uint32_t num);
int cnt_digits(uint32_t number);
void find_digits(int *dig, uint32_t number);

/*__********************************************************************
 ************************************************************************
 ************************************************************************
 **************************    * main  *   ******************************
 ************************************************************************
 ************************************************************************
 ********************************************************************__*/

static int NUM = 32;
int main(int argc, const char * argv[]) {
    
    printf("LIMIT: %u\n",LIMIT);
    printf("bitmap size : %u\n",(LIMIT/8)+1);
    unsigned int sqrt_LIMIT = sqrt(LIMIT)+1;

    //char mode = 't';
    
    if(mode == 'p'){
       NUM = NUM_PROCESSES;
       create_bitmap();
       printf("bitmap created\n");
       SEEDING_Primes(sqrt_LIMIT);

       printf("Starting processes ...\n");
       start_processes(sqrt_LIMIT, LIMIT);

       set_SEEDING_HappyPrimes(LIMIT);
       print_primes(LIMIT);
       printf("bitmap I think should still be around\n");
       
       if(shm_unlink("./Happy_Primes_bitmap")==-1)
       {
	  printf("Couldn't unlink Happy_Primes_bitmap\n");
	  exit(-1);
       }
       printf("unlinked shm\n");
    }
    else if(mode == 't'){
       NUM = NUM_THREADS;
       
       address = (char *) malloc(sizeof(char*)*((LIMIT/8)+1));
       
       SEEDING_Primes(sqrt_LIMIT);

       printf("Starting threads ... \n");

       start_threads(sqrt_LIMIT, LIMIT);

       free(address);
    }
    
    return 0;
}

/*__********************************************************************
 ************************************************************************
 ************************************************************************
 ****************    * Functions' Definition  *   ***********************
 ************************************************************************
 ************************************************************************
 ********************************************************************__*/

void create_bitmap()
{
    int fd = shm_open("./Happy_Primes_bitmap", O_RDWR|O_CREAT , S_IRUSR|S_IWUSR );
    if(fd == -1){
        printf("Can't open Happy_Primes_bitmap\n");
        exit(-1);
    }
    
    unsigned int length = (unsigned int) (LIMIT/8)+1; /*number of bytes needed*/
    
    if(ftruncate(fd,length)==-1){
        printf("Can't truncate Happy_Primes_bitmap to length %u\n",length);
        exit(-1);
    }
    
    address = mmap(NULL, length, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (address==MAP_FAILED) {
        printf("Can't mmap Happy_Primes_bitmap to any address\n");
        exit(-1);
    }
}

void set_NotPrime(unsigned int n) {
     address[WORD_OFFSET(n)] |= (1 << BIT_OFFSET(n));
}
void set_NotHappy(unsigned int n) {
    address[WORD_OFFSET(n)] |= (1 << BIT_OFFSET(n));
}
int check_Prime(unsigned int n) {
    char bit = address[WORD_OFFSET(n)] & (1 << BIT_OFFSET(n));
    return bit == 0;
}

void print_primes(unsigned int limit)
{
   unsigned int count=0;
    unsigned int i=0;
    for(i=0;i<limit; i++)
        if (check_Prime(i)) {
           count++; 
        }
    printf("Total happy primes : %u\n",count);
}

void SEEDING_Primes(unsigned int max)
{
    unsigned long long i;
    unsigned int base=2;
    unsigned int multiple=2;
    
    set_NotPrime(0);
    set_NotPrime(1);

    while(base<=sqrt(max) ){
        
        while(((unsigned long)base)*multiple<=max){
                set_NotPrime(base*multiple);
                multiple++;
        }
        multiple=2;
        base++;
        while(base<=sqrt(max) && check_Prime(base)==0){
                base++;
        }
    }

}

unsigned int set_SEEDING_HappyPrimes(unsigned int max)
{
   unsigned int i=0;
   unsigned int count=0;
   unsigned int countp=0;
   for(i=0;i<sqrt(max)+1;i++)
   {
      if(check_Prime(i))
      {
	 countp++;
	 if(check_happy(i)==0)
	 {
	   // printf("prime %u is NOT happy\n",i);
	    set_NotHappy(i);
   	    count++;
      	 }
      }
   }
 //  printf("I'm %u, I found between %u and %u : %u primes, %u happy primes\n",getpid(),0,i,countp,countp-count);
   return (countp - count);
}

void create_primes_que(struct info_package* pp, unsigned int start, unsigned int end)
{
   unsigned int i,j;
   unsigned int step= (end-start)/NUM;
   
   /*Add starting primes to the que of each process.*/
   for(i=0; i<start; i++)
   {
      if(check_Prime(i))
      {
	 for(j=0;j<NUM;j++)
	 {
	    struct node *newNode = (struct node*) malloc(sizeof(struct node));
	    newNode->prime = i;
	    newNode->next = pp[j].primes_que;
	    pp[j].primes_que = newNode;
	 }
      }
   }

   /*set the start and end for each process*/
   for(i=0; i<NUM; i++)
   {
      pp[i].pid = i;
      if(i==0){
	 pp[i].start = start;
      }
      else{
	 pp[i].start = start + (step*i) - ( (start+(step*i))%32 );
      }
      if(i!=(NUM-1)){
	 pp[i].end = start + (step*(i+1)) - 1 - ( (start+(step*(i+1)))%32 );
      }
      else{
	 pp[i].end = end;
      }
   }

}

void start_processes( unsigned int start, unsigned int end)
{
   struct info_package *pp = (struct info_package*) malloc(sizeof(struct info_package)*NUM);

   struct sigaction s, t;
   sigset_t blockSet, prevMask;
   sigemptyset(&blockSet);
   sigaddset(&blockSet, SIGINT);
   sigaddset(&blockSet, SIGQUIT);
   if(sigprocmask(SIG_BLOCK, &blockSet, &prevMask) == -1)
      exit(-1);
   
   create_primes_que(pp,start,end);

   unsigned int i=0;
   for(i=0; i<NUM; i++){
      sigaddset(&blockSet, SIGCHLD);
      if(sigprocmask(SIG_BLOCK, &blockSet, &prevMask) == -1)
      	 exit(-1);
      switch(fork()){
	 case -1 :
	    printf("Cant fork child #%u\n",i);
	    exit(-1);
	 case 0 :
      	    if(sigprocmask(SIG_BLOCK, &prevMask, NULL) == -1)
      	       exit(-1);
	    RunProcess(&pp[i]);
	    while(pp->primes_que!=NULL){
	       struct node* temp = pp->primes_que;
	       pp->primes_que = pp->primes_que->next;
	       free(temp);
	    }
	    exit(0);
	 default :
	    break;
      }
   }

   unsigned int sum_count=0;
   for(i=0;i<NUM;i++){
      int k;
      wait(&k);			
				                                                       
      while(pp->primes_que!=NULL){
	 struct node* temp=pp->primes_que;
	 pp->primes_que=pp->primes_que->next;
	 free(temp);
      }
   }
   free(pp);

   if(sigprocmask(SIG_SETMASK, &prevMask, NULL) == -1)
      exit(-1);
}

void start_threads( unsigned int start, unsigned int end)
{
   pthread_t * thr= (pthread_t*) malloc(sizeof(pthread_t)*NUM);
   struct info_package *pkg = (struct info_package*) malloc(sizeof(struct info_package)*NUM);

   create_primes_que(pkg,start,end);

   unsigned int i=0;
   for(i=0; i<NUM; i++){
   	pthread_create(&thr[i], NULL, RunThread, &pkg[i]);   
   }

   void *cnt;
   for(i=0;i<NUM;i++){
      pthread_join(thr[i],&cnt);
      sum_count = sum_count + (unsigned long long) cnt;
				                                                       
      while(pkg->primes_que!=NULL){
	 struct node* temp=pkg->primes_que;
	 pkg->primes_que=pkg->primes_que->next;
	 free(temp);
      }
   }
   printf("Thread, total Happy Primes: %llu\n", sum_count+set_SEEDING_HappyPrimes(LIMIT));
   free(pkg);
   free(thr);
}

unsigned int RunProcess(void* arg){
   
   struct info_package* pp=(struct info_package*)arg;
   struct node* current=pp->primes_que;
   
   unsigned long long size=1000000;
   unsigned long long i=0;
   unsigned long long mod,curStartIndex;
   unsigned int localCount=0;
   
   i=pp->start;

   for(curStartIndex=pp->start ;i < pp->end; curStartIndex= curStartIndex +size){
      current=pp->primes_que;
      while(current!=NULL ){
	 i=curStartIndex; 

	 if( (mod=i%current->prime) !=0)i=i+(current->prime-mod);

	 while(i <= (pp->end) && i< curStartIndex+size){
	    set_NotPrime(i);
	    i+=current->prime;
	 }

	 current=current->next;
      }
   }

   unsigned int nothappycount =0;
   for(i=pp->start;i<pp->end;i++){
      if(check_Prime(i))
      {
	 localCount++;
	 if(check_happy(i)==0)
	 {
	    set_NotHappy(i);
	 //   printf("prime %llu is NOT happy\n",i);
	    nothappycount++;
	 }
      }
   }
   printf("I'm child %u, I found %u between %u and  : %u primes, %u happy primes\n",getpid(),pp->start,pp->end,localCount,localCount-nothappycount);
   return localCount;
}

static void * RunThread(void* arg){
   
   struct info_package* pkg=(struct info_package*)arg;
   struct node* current=pkg->primes_que;
   
   unsigned long long size=1000000;
   unsigned long long i=0;
   unsigned long long mod,curStartIndex;
   unsigned int localCount=0;
   
   i=pkg->start;

   for(curStartIndex=pkg->start ;i < pkg->end; curStartIndex= curStartIndex +size){
      current=pkg->primes_que;
      while(current!=NULL ){
	 i=curStartIndex; 

	 if( (mod=i%current->prime) !=0)i=i+(current->prime-mod);

	 while(i <= (pkg->end) && i< curStartIndex+size){
	    set_NotPrime(i);
	    i+=current->prime;
	 }

	 current=current->next;
      }
   }

   unsigned int nothappycount =0;
   for(i=pkg->start;i<pkg->end;i++){
      if(check_Prime(i))
      {
	 localCount++;
	 if(check_happy(i)==0)
	 {
	    set_NotHappy(i);
	 //   printf("prime %llu is NOT happy\n",i);
	    nothappycount++;
	 }
      }
   }
   printf("I'm thread %u, I found %u between %u and  : %u primes, %u happy primes\n",pkg->pid,pkg->start,pkg->end,localCount,localCount-nothappycount);
   unsigned int happy = localCount - nothappycount;
   return (void*)happy;
}


/*************************************************************************
 **************  * Function name: find_happy_prime *  ********************
 * Usage: Finds prime numbers less than "limit" and prints the happy ones.
 **************************************************************************
 **************************************************************************/
void find_happy_prime(int *nprime,int cnt, uint32_t limit)
{
    int hp[cnt];
    int i=0;
    int count=0;
    for(i=0;i<cnt;i++)
    {
        hp[i] = check_happy(nprime[i]);
        if(hp[i]!=0)
        {
            //printf("Prime #%d is Happy\n",hp[i]);
            count++;
        }
    }
    printf("\nNumber of Happy Primes less than  %d is %d\n",limit,count);

}
/**************************************************************************
 **************  * Function name: p_print *  ******************************
 * Usage: Prints prime numbers stored in "nprime".
 **************************************************************************
 **************************************************************************/
void p_print(int *nprime,int cnt, uint32_t limit)
{
    for(int i=0;i<cnt;i++) // Print primes
    {
        printf("%d\n",nprime[i]);
    }
    
    printf("Number of Primes less than  %d is %d\n",limit,cnt);
}
/**************************************************************************
 **************  * Function name: find_primes *  **************************
 * Usage: Finds the prime numbers less than "limit" and stores them in "nprime".
 **************************************************************************
 **************************************************************************/
void find_primes(uint32_t limit, int *cnt, int *nprime)
{
    int i,j,c=0;
    int xprime[limit/2];
    int prime[limit];
    for(i=0 ;i<limit;i++) // init
    {
        prime[i]= i;
        //printf("prime[%d] = %d\n", i, prime[i]);
    }
    for(i=0;i<limit;i++) // Do Sieve
    {
        if( (prime[i]!=0)&&(prime[i]!=1) )
        {
            for(j=2;j<limit;j++)
            {
                {
                    if( (prime[i]*j) > limit )
                        break;
                    else
                        prime[j*prime[i]]=0;
                }
            }
        }
    }
    
    for(i=0;i<limit;i++) // Print primes
    {
        if(prime[i]!=0)
        {
            if(prime[i]!=1)
            {
                xprime[c] = prime[i];
                c++;
            }
        }
    }
    
    *cnt = c;
    for(i=0; i<c; i++)
    {
        nprime[i]=xprime[i];
    }
    
}
/**************************************************************************
 **************  * Function name: find_happy *  ***************************
 * Usage: Returns "num" if it is a happy number.
 **************************************************************************
 **************************************************************************/
uint32_t check_happy(uint32_t num)
{
    int sum =0;
    int i =0;
    int count =0;
    uint32_t temp = num;
    do
    {
        int count_digits = cnt_digits(temp);
        int *dig = (int*) malloc( count_digits * sizeof(int) );
        
        find_digits(dig, temp);
        sum =0;
        for(i=0; i<count_digits;i++)
        {
            //printf("%d = %d + (%d*%d)\n", sum , sum , dig[i],dig[i]);
            sum = sum + (dig[i]*dig[i]);
           // printf("sum = %d i = %d count_digits = %d\n",sum, i,count_digits);
        }
        
        temp = sum;
        count++;
        
        free(dig);
        if (temp == 4) /* if not happy, one if its sequence has to be 4 */
        {
            //printf("Prime #%u is NOT Happy\n",num);
            //printf("Couldn't find happy number after %d interation.\n",count);
            return 0;
        }
    }while(sum != 1);
    
    //printf("found happy number %u after %d interation.\n",num,count);
    return num;

}
/**************************************************************************
 **************  * Function name: find_digits *  **************************
 * Usage: Parse "number" to its digits and store that in int array "dig".
 **************************************************************************
 **************************************************************************/
void find_digits(int *dig, uint32_t number)
{
    uint32_t num = number;
    
    int count_digits = cnt_digits(num);

    int i = 0;
    int sum=0;
    //printf("Number: %d count_digits: %d\n",num, count_digits);
    int temp = count_digits;
    while(temp!=0)
    {
        if(temp>1)
        {
            sum = num % (int)pow(10,temp-1);
            int ncount_digits = temp -1;
            while(sum>9)
            {
                sum = num % (int)pow(10,ncount_digits-1);
                ncount_digits--;
            }
        }
        
        
        else
        {
            
            sum = num % 10;
        }
        dig[i++] = sum;
        //printf("dig[%d]: %d\n",(i-1), dig[i-1]);
        temp--;
        num = num /10;
    }

}
/**************************************************************************
 **************  * Function name: cnt_digits *  ***************************
 * Usage: Returns how many digits in "number".
 **************************************************************************
 **************************************************************************/
int cnt_digits(uint32_t number)
{
    int count_digits = 0;
    uint32_t temp = number;
    while(temp!=0)
    {
        temp =  temp / 10;
        count_digits++;
    }
    return count_digits;
}
