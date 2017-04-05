#define _POSIX_C_SOURCE 200908L
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_THREADS     8
#define LIMIT 1000000
volatile int sum = 0;


void *busy(void *threadid)
{
  long tid;
  tid = (long)threadid;
  
  
  printf("I'm thread #%ld, and I'm starting up.\n", tid);

  int local_sum = 0;
  for(int i=0; i<LIMIT; ++i)
    {
      local_sum++;
    }

  sum += local_sum;
  pthread_exit(NULL);
}

int main (int argc, char *argv[])
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

  printf("Sum = %d\n", sum);
  // The last thing; clean-up
  pthread_exit(NULL);
}
