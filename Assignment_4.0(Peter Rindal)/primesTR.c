#include <sys/types.h>
#include <utime.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>
#include <glob.h>
#include <ctype.h>
#include <pthread.h>
//#include <limit.h>    /* for CHAR_BIT */
#include <stdint.h>   /* for uint32_t */

typedef char word_t;
enum { BITS_PER_WORD = 8 };
#define WORD_OFFSET(b) ((b) / BITS_PER_WORD)
#define BIT_OFFSET(b)  ((b) % BITS_PER_WORD)
static	int NUM_THREADS = 10;
static word_t * primes;		// bitmap
static unsigned int bound=0-1;
unsigned long long count;

// prototypes
void compare();
void sentToThreads(unsigned int,unsigned int);
void getStartingPrimes(unsigned int max);
void set_NotPrime(unsigned int n);
void set_Prime(unsigned int n);
int check_Prime(unsigned int n);
int timing(struct timeval start);
void getArguments(int argc,char ** argv);
static void * threadRun(void* arg);
unsigned int recursivelyGetStartingPrimes(unsigned int max);

// holds the information a thread will need.
struct threadPackage{
	unsigned int start;	// starting location the thread should work in.
	unsigned int end;	// ending location the thread should work in/
	struct node * que;	// a que of primes that a thread will use to find more primes/
	int tid;	//thread id. used in debugging only.
};

// used as a way to que the primes the threads will use as seeds.
struct node{
	unsigned int prime;	// the prime that is stored in this node.
	struct node * next;	// the next prime in the que.
};

int main(int argc, char** argv){
	struct timeval startTime;
	gettimeofday(&startTime,NULL);	// gets the starting time
	
	// gets the upper bound and number of threads to use
	getArguments(argc,argv);

	// creates the bitmap to store the primes
	primes = (char*)malloc(sizeof(char)*((bound/BITS_PER_WORD) +1));

	// gets the primes that will be used as seeds for the threads
	unsigned int r=recursivelyGetStartingPrimes(sqrt(bound));

	// creates and runs the threads
	sentToThreads(r+1,bound);

	// does the timing on the program
	timing(startTime);

	compare();
	return 1;
}

/*	Summary : argArguments

	reads the command ling arguments and sets the coresponding veriables

	input:	argc - number of command line arguments
			argv - the command line arguments

	sets NUM_THREADS
	sets bound

	bound is the upper bound that the program will look for
*/
void getArguments(int argc,char ** argv){
	char* end;
	if(argc>=2){
		NUM_THREADS=strtol(argv[1],&end,10);
		if(NUM_THREADS==0)NUM_THREADS++;
	}
	if(argc>=3){
		double temp=strtod(argv[2],&end);
		if(temp >4294967296){
			bound=4294967295;
			printf("%f is too large. range has been shortened to %u\n",temp,bound);
		}else bound=(unsigned int)temp;
		if(bound==0)bound--;
		
	}
	
}

unsigned int recursivelyGetStartingPrimes(unsigned int max){
	if(max<=2){ // base case

		printf("base Case %u\n",max);
		fflush(stdout);
		set_NotPrime(0);
		set_NotPrime(1);
		count++;
		return 2;
	}
	unsigned int r =recursivelyGetStartingPrimes(sqrt(max));
	printf("sending to threads with min=%u max=%u\n",r+1,max);
	fflush(stdout);
	sentToThreads(r+1,max);
	return max;
}

/*	Summary : makeQues

	generates the information that each thread will need. This includes where
	a thread should start and where it should end in the bitmap. it also genertes 
	a list of all the primes that the thread should use as a seed.

	input:	tp - a pointer to the array that the thread information should be places in
			start - the first number that the threads should consider to be a prime or not.
					all non primes before start should have have been set as not prime.
			end - the last number the threads should consider to be a prime or not.
	
*/
void makeQues(struct threadPackage* tp,unsigned int start,unsigned int end){
	unsigned int i,j;
	unsigned int step= (end-start)/NUM_THREADS;
	//printf("step=%u\n",step);
	if(step<8)step=8;
	for(j=0;j<NUM_THREADS;j++){
		tp[j].que=NULL;
	}
	// adds all the primes before start to each threadPackage que
	for(i=0;i<start;i++){
		if(check_Prime(i)){
			//printf("quing %u\n",i);
			for(j=0;j<NUM_THREADS;j++){
				struct node* newNode=(struct node*)malloc(sizeof(struct node));
				newNode->prime=i;
				newNode->next=tp[j].que;
				tp[j].que=newNode;
			}
		}
	}

	// calculates the range ech thread chould concider
	for(i=0;i<NUM_THREADS;i++){
		tp[i].tid=i;
		if(i==0){
			tp[i].start=start+step*i;
		}else{
			// makes sure that a process doesnt start in the middle of a unit of data
			tp[i].start=start+step*i-((start+step*i)%8);	
			//if(tp[i].start<start)tp[i].start=start;
		}
		if(i!=NUM_THREADS-1){
			// makes sure that a process doesnt end in the middle of a unit of data
			tp[i].end=start+step*(i+1)-1-((start+step*(i+1))%8);
			if(tp[i].end>end)tp[i].end=end;
		}else{
			tp[i].end=end;
		}
		if(i!=0){
			if(tp[i-1].end==tp[i].end || tp[i-1].end==0)tp[i].end=0;
		}
	}
}

/*	Summary : sentToThreads

	this method creates the threads and gets all the information each one will need to execute.
	once all the threads have finished their task they are reaped by this method.

	input:	start - the first number that the threads should consider to be a prime or not.
					all non primes before start should have have been set as not prime.
			end - the last number the threads should consider to be a prime or not.
	
*/
void sentToThreads(unsigned int start,unsigned int end){
	unsigned int i,j;
	
	// creates the space to hold the thread handlers
	pthread_t * threads=(pthread_t*)malloc(sizeof(pthread_t)*NUM_THREADS);

	// creates the space for the information the threads will need
	struct threadPackage* tp=(struct threadPackage*)malloc(sizeof(struct threadPackage)*NUM_THREADS);
	
	// generates the information the threads will need
	makeQues(tp,start,end);
	
	for(i=0;i<NUM_THREADS;i++){
		//	creates a thread and sends it its own personal 
		//  thread info in the threadPackage  tp[i]
		//printf("creating thread%u with start=%u end=%u\n",tp[i].tid,tp[i].start,tp[i].end);
		if(tp[i].end!= 0){
			pthread_create(&threads[i],NULL,threadRun,&tp[i]);
		}
	}
	
	void *res;
	for(i=0;i<NUM_THREADS;i++){
		
		// reaps the threads when they have completed.
		if(tp[i].end!= 0){
			pthread_join(threads[i],&res);
			count= count +(unsigned long long)res;
		}

		
		// frees the numbers in the threadPackage que
		while(tp->que!=NULL){
			struct node* temp=tp->que;
			tp->que=tp->que->next;
			free(temp);
		}
	}
	printf("count=%llu\n",count);
	free(tp);
	free(threads);
}

/*	Summary : getStartingPrimes
	
	this method generates all of the primes that will be need as seeds for the threads.
	it does this by using the Sieve of Eratosthenes algorithm to calculate all the primes.

	input:	max - the maximum value that this mether should calculate to be a prime or not.
*/
void getStartingPrimes(unsigned int max){
	// this is the Sieve of Eratosthenes algorithm.
	unsigned int base=2;
	unsigned long long i;
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
	for(i=2;i<= max;i++){
		if (check_Prime(i))count++;
	}
}

/*	Summary : set_NotPrime

	this method sets the bit at index n to represent not being prime. when the bitmap 
	it initialized all bits are set as prime. only non prime numbers should be altered

	input:	n - the number that is to be set as not prime.
			
*/
void set_NotPrime( unsigned int n) { 
    primes[WORD_OFFSET(n)] |= (1 << BIT_OFFSET(n));
}

/*	Summary : check_Prime

	this method returns a int that tells you whether the input number has been set as 
	prime or not. note that  when the bitmap it initialized all bits are set as prime.
	this method should only be used on numbers that have already been calculated as 
	prime or not.

	input:	n - the number that is to be checked as prime or not.

	returns: int 0 - if the input is not prime.
			 non zero integer - if the number is prime or if it has not been calculated yet.
			
*/
int check_Prime(unsigned int n) {	
    word_t bit = primes[WORD_OFFSET(n)] & (1 << BIT_OFFSET(n));
    return bit == 0; 
}

/*	Summary : threadRun

	this method is ment to be run as the main program in the child threads. 
	It takes a void pointer that points to location of the threadPackage struct 
	that holds the information that this method need to run. This includes the range 
	that this method should calculate the primes on and all of the primes that will 
	act as seeds. These primes are held in a linked list. 

	to increase the performace of this method there are three loops all nested inside 
	of eachother. The outer loop is need is the loop that increases performance. It 
	loops over this threads range as defined in the threadPackage tp in increments of
	'size'. Size is set to be 1 million currently.

	The middle loop starts with the first prime provided in the threadPackage tp and 
	loops the the que untill there are no more primes

	The inner loop set all of the multiples of the current prime given by the middle 
	loop as not prime. this loop stops when it either reaches the end of the over range
	given by the threadPackage or when it reaches the end of the current 1 million that 
	are being considered

	input:	arg - a void pointer to a threadPackage.

	returns: 123 - this means nothing.
*/
static void * threadRun(void* arg){
	struct threadPackage* tp=(struct threadPackage*)arg;
	struct node* current=tp->que;
	unsigned long long size=1000000;	//loop size
	unsigned long long i=0;
	unsigned int localCount=0;
	unsigned long long mod,curStartIndex;
	i=tp->start;

	/*	loops over the thread's section in increments == size*/
	for(curStartIndex=tp->start ;i < tp->end; curStartIndex= curStartIndex +size){	
		// gets first prime from the que
		current=tp->que;		

		while(current!=NULL ){
			i=curStartIndex;	// the start of this increment
			
			//places i on the first multiple of the current prime in this section
			if( (mod=i%current->prime) !=0)i=i+(current->prime-mod);		
			
			// loops through the section and sets all multiples of the prime as not prime
			while(i <= (tp->end) && i< curStartIndex+size){
				set_NotPrime(i);
				i+=current->prime;
			}
			// gets next prime from the que
			current=current->next;	
		}
	}
	
	// counts the primes in this threads section.
	for(i=tp->start;i<=tp->end;i++){
		if(check_Prime(i))localCount++;
	}
	return (void*)localCount; // returns the number of primes this thread found in its section.
}

/*			*\
	EXTRAS
\*			*/
unsigned int countPrimes(){
	unsigned long long i,k;
	
	k=0;
	for(i=0;i<=bound;i++){
		
		if(check_Prime(i)){
			k++;
		}
	}
	
	return (unsigned int)k;
}

void compare(){
	int numRead,h=10;
	unsigned int Buff2=0,last=0;
	unsigned long long i,k=0,j=0,p,l=0;
	
	int o=open("/nfs/stak/students/r/rindalp/public_html/primesCOMPLETE.32b",O_RDONLY,S_IRUSR|S_IWUSR);
	if(o==-1){
		printf("open fail");
		exit(-1);	
	}
	numRead=4;
	printf("comparing your primes |           |\n                       ");
	fflush(stdout);
	while(numRead==4 ){
		numRead=read(o,&Buff2,numRead);
		if(numRead==-1){
			printf("read fail");
			exit(-1);	
		}
		
		while(last<Buff2 || ( last<=bound && numRead==0) ){
			if(check_Prime(last)){
				if(last!=2){
					printf("\nprime %u isnt prime\n",last);
				}else j++;
			}else if(last ==2){
				printf("\nprime %u isnt present\n",last);
			}else k++;

			last++;
		}
		last++;

		if(Buff2>bound || numRead==0)break;
		p=(j+k)%(bound/11);
		if(p<l){
			printf("*");
			fflush(stdout);
		}
		l=p;
		if(!check_Prime(Buff2)){
			printf("\nprime %u isnt present\n",Buff2);
		}
		j++;
	}
	
	printf("\nchecked all %llu primes and all %llu nonprimes\n",j,k);
	
	
	
}

int timing(struct timeval start){
	struct timeval t;
	int c=gettimeofday(&t,NULL);
	if(c==-1){
		printf("couldnt get ending time");
		exit(-1);
	}
	time_t diff= t.tv_sec-start.tv_sec;
	suseconds_t micro=t.tv_usec-start.tv_usec;
	int o=open("primesT_timing.tsv",O_RDWR |O_APPEND |O_CREAT,S_IRUSR|S_IWUSR);
	if(o==-1){
		printf("couldnt open primesT_timing.tsv\n");
		exit(-1);
	}
	char lineBuff[30];
	char microBuff[6];
	char temp[6];
	int nDigits;
	if(micro<0){
		micro=1000000-micro;
		diff--;
	}
	if(micro!=0)nDigits = floor(log10(abs(micro))) + 1;
	else nDigits=1;
	for(c=0;c<6-nDigits;c++){
		microBuff[c]='0';
	}
	sprintf(temp,"%ld",(long)micro);
	for(c=6-nDigits;c<6;c++){
		microBuff[c]=temp[c-(6-nDigits)];
	}

	sprintf(lineBuff,"%u\t%u\t%ld.%s\n",NUM_THREADS,bound,(long)diff,microBuff);
	printf("timing:%s",lineBuff);

	int numWritten=write(o,lineBuff,strlen(lineBuff));
	if(numWritten==-1){
		printf("couldnt write to primesT_timing.tsv\n");
		exit(-1);
	}
	return 1;
}