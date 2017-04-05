#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char* argv[])
{
	
  printf("wassap I'm main1.c\n"); 
//  printf("%s\n%s\n", argv[0], argv[1]); 
  printf("\n%s: %s\n", argv[1], getenv(argv[1])); 
//  system ("ls -l");
  return 0;
}
