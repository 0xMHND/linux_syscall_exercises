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
#include <sys/mman.h>
//#include <limit.h>    /* for CHAR_BIT */
#include <stdint.h>   /* for uint32_t */

//typedef uint32_t word_t;
enum { BITS_PER_WORD = sizeof(char)*8 };
#define WORD_OFFSET(b) ((b) / BITS_PER_WORD)
#define BIT_OFFSET(b)  ((b) % BITS_PER_WORD)
static  int NUM_PROCESSES = 10;
static char* addr;		// bitmap
static unsigned int bound=0-1;

// prototypes

void compare();
void sentToProcesses(unsigned int,unsigned int);
void getStartingPrimes(unsigned int max);
void set_NotPrime(unsigned int n);
void set_Prime(unsigned int n);
int check_Prime(unsigned int n);
int timing(struct timeval start);
void getArguments(int argc,char ** argv);
unsigned int processRun(void* arg);
void mountSHM();
unsigned int countPrimes();

// holds the information a child process will need.
struct threadPackage{
	unsigned int start;	// starting location the thread should work in.
	unsigned int end;	// ending location the thread should work in/
	struct node * que;	// a que of primes that a thread will use to find more primes/
	int tid;	//process id. used in debugging only.
};

// used as a way to que the primes the processes will use as seeds.
struct node{
	unsigned int prime;	// the prime that is stored in this node.
	struct node * next;	// the next prime in the que.
};

int main(int argc, char** argv){
	struct timeval startTime;
	int ff=gettimeofday(&startTime,NULL); // gets the starting time

	// gets the upper bound and number of processes to use
	getArguments(argc,argv);

	// creates the bitmap to store the primes
	mountSHM();

	// gets the primes that will be used as seeds for the threads
	getStartingPrimes(sqrt(bound));
	
	// creates and runs the processes
	sentToProcesses(sqrt(bound)+1,bound);

	//compare();
	// prints the number of primes
	printf("count=%u\n",countPrimes());
	// does the timing on the program
	//timing(startTime);
	
	// unlinks the shared memory object.
	if(shm_unlink("/Peter_Rindal_Primes_SMO") == -1){
		printf("couldnt unlink /Peter_Rindal_Primes_SMO \n");
		exit(-1);
	}
	return 1;
}

/*	Summary : mountSHM

	mounts a shared memory object that will be used as a bitmap to hold 
	whether a number is prime or not.
*/
void mountSHM(){

	int fd= shm_open("/Peter_Rindal_Primes_SMO",O_RDWR | O_CREAT,S_IRUSR|S_IWUSR);
	if(fd==-1){
		printf("couldnt shm_open /Peter_Rindal_Primes_SMO \n");
		exit(-1);
	}
	unsigned int length = (unsigned int) (bound/8)+1;
	if(ftruncate(fd,length)==-1){
		printf("couldnt truncate /Peter_Rindal_Primes_SMO to length %u\n",length);
		exit(-1);
	}
	addr = mmap(NULL,length,PROT_READ | PROT_WRITE, MAP_SHARED,fd,0);
	if(addr == MAP_FAILED){
		printf("couldnt mmap /Peter_Rindal_Primes_SMO \n");
		exit(-1);
	}

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
		NUM_PROCESSES=strtol(argv[1],&end,10);
		if(NUM_PROCESSES==0)NUM_PROCESSES++;
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
	unsigned int step= (end-start)/NUM_PROCESSES;
	
	// adds all the primes before start to each threadPackage que
	for(i=0;i<start;i++){
		if(check_Prime(i)){
			for(j=0;j<NUM_PROCESSES;j++){
				struct node* newNode=(struct node*)malloc(sizeof(struct node));
				newNode->prime=i;
				newNode->next=tp[j].que;
				tp[j].que=newNode;
			}
		}
	}

	// calculates the range ech thread chould concider
	for(i=0;i<NUM_PROCESSES;i++){
		tp[i].tid=i;
		if(i==0){
			tp[i].start=start+step*i;
		}else{
			// makes sure that a process doesnt start in the middle of a unit of data
			// although my bitmap units are 8 bits long i left them as 32 because im too lazy
			// rewrite the code and 32 is a multiple of 8 so its the same thing at the end of 
			// the day
			tp[i].start=start+step*i-((start+step*i)%32);	
		}
		if(i!=NUM_PROCESSES-1){
			// makes sure that a process doesnt end in the middle of a unit of data
			tp[i].end=start+step*(i+1)-1-((start+step*(i+1))%32);
		}else{
			tp[i].end=end;
		}
	}
}

/*	Summary : sentToProcesses

	this method creates the processes and gets all the information each one will need to execute.
	once all the pocesses have finished their task they are reaped by this method.

	input:	start - the first number that the pocesses should consider to be a prime or not.
					all non primes before start should have have been set as not prime.
			end - the last number the pocesses should consider to be a prime or not.
*/
void sentToProcesses(unsigned int start,unsigned int end){
	unsigned int i;
	
	// creates the space for the information the processes will need. 
	// uses a threadPackage be because im too lazy to rewrite all the name to processPackage. 
	struct threadPackage* tp=(struct threadPackage*)malloc(sizeof(struct threadPackage)*NUM_PROCESSES);
	
	// generates the information the threads will need
	makeQues(tp,start,end);
	
	for(i=0;i<NUM_PROCESSES;i++){
		//	creates a child process and sends it its own personal 
		//  process info in the threadPackage  tp[i]
		switch(fork()){
		case -1:	// error
			printf("couldnt fork process %u\n",i);
			exit(-1);
		case 0:		// child
			processRun(&tp[i]);
			while(tp->que!=NULL){
				struct node* temp=tp->que;
				tp->que=tp->que->next;
				free(temp);
			}
			exit(1);
		default:	// parent
			break;
		}
	}

	for(i=0;i<NUM_PROCESSES;i++){
		int k;

		// reaps the threads when they have completed.
		wait(&k);
		
		// frees the numbers in the threadPackage que
		while(tp->que!=NULL){
			struct node* temp=tp->que;
			tp->que=tp->que->next;
			free(temp);
		}
	}
	
	free(tp);
	
}

/*	Summary : getStartingPrimes
	
	this method generates all of the primes that will be need as seeds for the processes.
	it does this by using the Sieve of Eratosthenes algorithm to calculate all the primes.

	input:	max - the maximum value that this mether should calculate to be a prime or not.
*/
void getStartingPrimes(unsigned int max){
	unsigned long long i;
	unsigned int base=2;
	unsigned int multiple=2;
	set_NotPrime(0);
	set_NotPrime(1);
			
	while(base<=sqrt(max) ){
		//printf("%u\n",base);
		//fflush(stdout);
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

/*	Summary : set_NotPrime

	this method sets the bit at index n to represent not being prime. when the bitmap 
	it initialized all bits are set as prime. only non prime numbers should be altered

	input:	n - the number that is to be set as not prime.	
*/
void set_NotPrime(unsigned int n) { 
    addr[WORD_OFFSET(n)] |= (1 << BIT_OFFSET(n));
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
	
    char bit = addr[WORD_OFFSET(n)] & (1 << BIT_OFFSET(n));
    return bit == 0; 
}

/*	Summary : processRun

	this method is ment to be run as the main program in the child processes. 
	It takes a void pointer that points to location of the threadPackage struct 
	that holds the information that this method need to run. This includes the range 
	that this method should calculate the primes on and all of the primes that will 
	act as seeds. These primes are held in a linked list. 

	to increase the performace of this method there are three loops all nested inside 
	of eachother. The outer loop is need is the loop that increases performance. It 
	loops over this threads range as defined in the threadPackage tp in increments of
	'size'. Size is set to be 1 million currently. 

	The middle loop starts with the first prime provided in the threadPackage tp and 
	loops over the que untill there are no more primes

	The inner loop set all of the multiples of the current prime given by the middle 
	loop as not prime. this loop stops when it either reaches the end of the over range
	given by the threadPackage or when it reaches the end of the current 1 million that 
	are being considered

	input:	arg - a void pointer to a threadPackage.

	returns: 123 - this means nothing.
*/
unsigned int processRun(void* arg){
	struct threadPackage* tp=(struct threadPackage*)arg;
	struct node* current=tp->que;
	unsigned long long size=1000000;
	unsigned long long i=0;
	unsigned long long mod,curStartIndex;
	unsigned int localCount=0;
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
	// counts the primes in this processes section.
	for(i=tp->start;i<tp->end;i++){
		if(check_Prime(i))localCount++;
	}
	// returns the number of primes this process found in its section.
	// but it isnt used in the process version because im too lazy to 
	// use semaphores to add it to the global count;)
	return localCount; 
}

/*	Summary : countPrimes

	counts the bnumber of times marked in the bitmap.

	returns: the number of times.
*/
unsigned int countPrimes(){
	unsigned long long i,k;
	
	k=0;
	for(i=0;i<4294967296;i++){
		
		if(check_Prime(i)){
			k++;
		}
	}
	
	return (unsigned int)k;
}

/*			*\
	EXTRAS
\*			*/
int timing(struct timeval start){
	struct timeval t;
	int c=gettimeofday(&t,NULL);
	if(c==-1){
		printf("couldnt get ending time");
		exit(-1);
	}
	time_t diff= t.tv_sec-start.tv_sec;
	suseconds_t micro=t.tv_usec-start.tv_usec;
	int o=open("primesP_timing.tsv",O_RDWR |O_APPEND |O_CREAT,S_IRUSR|S_IWUSR);
	if(o==-1){
		printf("couldnt open primesP_timing.tsv\n");
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

	sprintf(lineBuff,"%u\t%u\t%ld.%s\n",NUM_PROCESSES,bound,(long)diff,microBuff);
	printf("timing:%s",lineBuff);

	int numWritten=write(o,lineBuff,strlen(lineBuff));
	if(numWritten==-1){
		printf("couldnt write to primesT_timing.tsv\n");
		exit(-1);
	}
	return 1;
}

void compare(){
	int numRead,i;
	unsigned int Buff2=0,j=0,k=0;
	int o=open("/nfs/stak/students/r/rindalp/public_html/primesCOMPLETE.32b",O_RDONLY,S_IRUSR|S_IWUSR);
	if(o==-1){
		printf("open fail");
		exit(-1);	
	}
	numRead=4;

	while(numRead==4){
		numRead=read(o,&Buff2,numRead);
		if(numRead==-1){
			printf("read fail");
			exit(-1);	
		}
		if(!check_Prime(Buff2)){
			printf("prime %u isnt present\n",Buff2);
			exit(-1);
		}
		j++;
		
	}
	printf("compare checked %u primes \n",j);


}