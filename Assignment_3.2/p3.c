#define _POSIX_C_SOURCE 200809L
#define _BSD_SOURCE

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>


extern void psignal(int sig, const char *msg);
extern const char *const sys_siglist[];

void sig_int_handler(int signal)
{
  psignal(signal, "\nA sign!");
}

int main(int argc, char *argv[])
{
  struct sigaction s;
  struct sigaction t;

  s.sa_handler = sig_int_handler;
  
  sigemptyset(&s.sa_mask);

  s.sa_flags = 0;

  // Install the handler.
  
  sigaction(SIGUSR1, &s, &t);
  
  for(int i=1;;++i){
    printf("Just swimming, %d.\n", i);
    sleep(2);
    if(i % 5 == 0)
      {
	raise(SIGUSR1);
	//kill(getpid(),SIGUSR1);
      }
  }
}
