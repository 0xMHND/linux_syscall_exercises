// Count the number of primes less than NUM and list them
#include <stdio.h>

#define NUM 1000

int main(void)
{
    int i,j,cnt=0;
    int num=NUM;
    int prime[NUM];

    for(i=0;i<num;i++) // init
    {
        prime[i]=i;
    }
    for(i=2;i<num;i++) // Do Sieve
    {
        if(prime[i]!=0)
        {
            for(j=2;j<num;j++)
            {
                {
                    if( (prime[i]*j) > num )
                        break;
                    else
                        prime[j*prime[i]]=0;
                }
            }
        }
    }
    for(i=0;i<num;i++) // Print primes
    {
        if(prime[i]!=0)
        {
            if(prime[i]!=1)
            { cnt+=1;
                printf("%d\n",prime[i]);
            }
        }
    }
    printf("Number of Primes less than %d is %d\n",NUM,cnt);
    return 0;
    
}
