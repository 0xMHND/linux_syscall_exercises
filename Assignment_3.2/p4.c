#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int main()
{

   int i=0;
   while(i<10)
   {
      printf("Swimming .. %d",i++);
      sleep(1);
   }
   return 1;
}

