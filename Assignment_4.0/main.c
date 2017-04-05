#define _POSIX_C_SOURCE 200908L
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define NUM_THREADS     8
#define NUM_PROCESSES 2
volatile int sum[2];

#define NUM sizeof(uint32_t)


/*__********************************************************************
************************************************************************
************************************************************************
******************    * Functuins Decleration *   **********************
************************************************************************
************************************************************************
********************************************************************__*/

/*Find primes and Print them*/
void find_primes(uint32_t limit, int *cnt, int *nprime);
void p_print(int *nprime,int cnt, uint32_t limit);


void *busy(void *threadid);
void thr_main();
/*__********************************************************************
************************************************************************
************************************************************************
**************************    * main  *   ******************************
************************************************************************
************************************************************************
********************************************************************__*/

int main(void)
{
  uint32_t LIMIT = 20; 
  int cnt=0;
  int *prime = (int*) malloc( LIMIT/2 * sizeof(int) );
  find_primes(LIMIT,&cnt,prime);
  p_print(prime,cnt, LIMIT);
  free(prime);
  printf("waiting ...\n");

  //thr_main();

  return 0;
    
}

/*__********************************************************************
************************************************************************
************************************************************************
****************    * Functions' Definition  *   ***********************
************************************************************************
************************************************************************
********************************************************************__*/

void p_print(int *nprime,int cnt, uint32_t limit)
{
  for(int i=0;i<cnt;i++) // Print primes
    {
       printf("%d\n",nprime[i]);
    }

  printf("Number of Primes less than  %d is %d\n",limit,cnt);
}

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







void *busy(void *threadid)
{
  long tid;
  tid = (long)threadid;
  
  
  printf("I'm thread #%ld, and I'm starting up.\n", tid);

  int local_sum = 0;
//  for(int i=0; i<LIMIT; ++i)
  //  {
    //   sum[i];
//      local_sum++;
   // }

//  sum += local_sum;
  pthread_exit(NULL);
}

void thr_main ()
{
  pthread_t threads[NUM_THREADS];
  int rc;
  long t;
  for(t=0; t<NUM_THREADS; t++){
    printf("Creating thread %ld\n", t);
    rc = pthread_create(&threads[t], NULL, busy, (void *)t);
    if (rc){
      printf("ERROR; return code from pthread_create() is %d\n", rc);
      exit(EXIT_FAILURE);
    }
  }
  
  for(t=0; t<NUM_THREADS; t++){
    pthread_join(threads[t], (void**)NULL);
  }

  printf("Sum = %d\n", sum[0]);
  // The last thing; clean-up
  pthread_exit(NULL);
}
