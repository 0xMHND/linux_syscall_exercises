#define _POSIX_C_SOURCE 200908L

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <sys/wait.h>
/*__********************************************************************
************************************************************************
******************    * Functuins Decleration *   **********************
************************************************************************
************************************************************************
********************************************************************__*/

int loop(char **cmdline, int *count);
void _system(char **cmdline);
int find_command(char *command, char** path);
int execute(char **cmdline);
char **split_line(char *line, int *cnt, int *mode);
int rdrctn(char **cmdline, int cnt); //done..
int _pipe(char **cmdline, int cnt);
void my_handler(int signum);

/*__********************************************************************
************************************************************************
************                    Main                       *************
************************************************************************
************************************************************************
********************************************************************__*/

int main(int argc, char* argv[])
{
   char *cmdline[50];
   int i=1,count;
   
   
   while(i != 0){
      i=loop(cmdline,&count);
   }
   
   return 0;
}
/*__********************************************************************
************************************************************************
******************    * Functuins Defintions *   ***********************
************************************************************************
************************************************************************
********************************************************************__*/
int loop(char **cmdline, int *count)
{
   char line[100];
   char **args;
   int i=0,cnt,mode=3; 
  /* Get input */ 
   printf("$msh> ");
   fgets(line, 100, stdin);
  
  /* parse input */ 
   args = split_line(line,&cnt,&mode);
   
  /* copy parsed line */ 
   while(i<cnt)
   {
      cmdline[i]=args[i];
      i++;
   }
   *count=cnt;

   cmdline[i]=NULL;/* Null terminate at the end */

   if(!strcmp(cmdline[0],"exit")||!strcmp(cmdline[0],"Exit"))
   {
      printf("Exiting ...\n");
      return 0; 
   }
   
   /*Signal Handling*/
   struct sigaction s;
   struct sigaction t;

   s.sa_handler = my_handler;
   sigemptyset(&s.sa_mask);
   s.sa_flags = 0;

   sigaction(SIGINT, &s, &t);
   sigaction(SIGQUIT, &s, &t);

   /*Redirecting*/
   if(mode == 0) // cmd < input
      return rdrctn(cmdline,cnt);
   if(mode == 1) // cmd > output
      return rdrctn(cmdline,cnt);
  
   /*Piping*/
   if(mode == 4) // cmd | cmd
      return _pipe(cmdline,cnt);
   
   /*regular commands*/
   if(mode == 3)
   {
      if(!fork()) 
      {
	 _system(cmdline);
	 printf("Not a valid command\n");
   	 return 0;
      }
      wait(&i);
      return 1;
   }
   return 1;
}
/**************************************************************************
**************  * Function name: _system *   ******************************
* Usage: My own system(), it prompt the user as a shell and then executes
* what he writes.
****************************************************************(**********
**************************************************************************/


void _system(char **cmdline)
{

   /* Execute command with options */
   if(!execute(cmdline))
   {
      printf("SORRY, CAN'T EXECUTE\n");
   }

}

/**************************************************************************
**************  * Function name: execute *   ******************************
* Usage: Execute command .
****************************************************************(**********
**************************************************************************/

int execute(char **cmdline)
{
   char *path;
   if( !(find_command(cmdline[0],&path)) )
   {
      printf("Not a valid command\n");
      return 0;
   }
  
   strcat(path,"/");
   strcat(path,cmdline[0]);
   
   execv(path,cmdline);//don't foregt to null cmdline
   return 1;
}

 
/**************************************************************************
**************  * Function name: find_command *   *************************
* Usage: Check if command exists and pass its path to 'path' .
***************************************************************************
**************************************************************************/

#define BSIZE 128
int find_command(char *command, char** path)   
{
   char buf[BSIZE];
   
   FILE *f = fopen("config","r");

   int i = 0;
   for(i=0; i<BSIZE; i++)
      buf[i] = '\0';

   while(!feof(f))
   {
      fgets(buf,BSIZE,f);
      if(!strcmp(buf,"PATH\n"))
      {
	 
         fgets(buf,BSIZE,f);
	 fclose(f);
	 break;
      }
   }
   
   if(buf == NULL)
   {
      printf("Didn't find any path\n");
      fclose(f);
      return 0;
   }
   
   int cnt=0;
   i=0;
   
   char **tokens = split_line(buf,&cnt,&i);
   
   for(int i=0;i<cnt;i++)
   {
      DIR *dp;
      struct dirent *ep;     
      dp = opendir (tokens[i]);
      if (dp != NULL)
      {
	 while (ep = readdir (dp))
	 {
	    if(!strcmp((ep->d_name),command) )
	    {
	       *path = tokens[i];
	    }
	 }
	 (void) closedir (dp);
      }
   }
   return 1;
}

 
/**************************************************************************
**************  * Function name: split_line *   ***************************
* Usage: Read a line and splits it and returns an array of strings .
****************************************************************(**********
**************************************************************************/


#define FSIZE 64
#define DELIM " \n:"
char **split_line(char *line, int *cnt, int *mode)
{
  int bufsize = FSIZE, pos = 0;
  char **tkns = malloc(bufsize * sizeof(char*));
  char *tkn;

  if (!tkns) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  tkn = strtok(line, DELIM);
  while (tkn != NULL) {
    tkns[pos] = tkn;
    if(!strcmp(tkns[pos],"<")) /* set the mode that cmdline[i-1] gets input from cmdline[i+1] output*/
       *mode = 0;
    if(!strcmp(tkns[pos],">")) /* set the mode that cmdline[i+1] gets input from cmdline[i-1] output */
       *mode = 1;
    if(!strcmp(tkns[pos],"|")) /* set the mode to a pipe moed. */
       *mode = 4;

    pos++;

    if (pos >= bufsize) {
      bufsize += FSIZE;
      tkns = realloc(tkns, bufsize * sizeof(char*));
      if (!tkns) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    tkn = strtok(NULL, DELIM);
  }
  tkns[pos] = NULL;
  *cnt = pos;
  return tkns;
}

/**************************************************************************
**************  * Function name: rdrctn *   ***************************
* Usage: performs redirection .
****************************************************************(**********
**************************************************************************/

int rdrctn(char **cmdline, int cnt)
{
   int i=0,mode=2;
   int j;
   char *new_cmdline[cnt-1];
   while(i<cnt)
   {
      if(!strcmp(cmdline[i],"<")) /* set the mode that cmdline[i-1] gets input from cmdline[i+1] output*/
      {
	 j=0;
	 while(j<i)
	 {
	    new_cmdline[j] = cmdline[j];
	    j++;
	 }
	 mode = 0;
	 break;
      }
      else if(!strcmp(cmdline[i],">")) /* set the mode that cmdline[i+1] gets input from cmdline[i-1] output */
      {
	 j=0;
	 while(j<i)
	 {
	    new_cmdline[j] = cmdline[j];
	    j++;
	 }
	 mode = 1;
	 break;
      }
      i++;
   }

   new_cmdline[j]=NULL;

   if(mode == 1)
   {
      int fd;
      if(!fork())
      {
	 fd = open(cmdline[i+1],O_RDWR|O_CREAT,S_IRWXU);
	 dup2(fd,1);
	 _system(new_cmdline);
	 close(fd);
	 return 0;
      }
      wait(&j);
      return 1;
   }
   if(mode == 0)
   {
      int fd;
      if(!fork())
      {
	 fd = open(cmdline[i+1],O_RDWR|O_CREAT,S_IRWXU);
	 dup2(fd,0);
	 _system(new_cmdline);
	 close(fd);
	 return 0;
      }
      wait(&j);
      return 1;
   }

   return 1;
}
/**************************************************************************
**************  * Function name: pipe *   ***************************
* Usage: performs piping .
****************************************************************(**********
**************************************************************************/

int _pipe(char **cmdline, int cnt)
{
   int i=0,mode=2;
   int j;
   char *first_cmdline[cnt-1];
   char *second_cmdline[cnt-1];
   while(i<cnt)
   {
      if(!strcmp(cmdline[i],"|")) /* set the mode that cmdline[i-1] gets input from cmdline[i+1] output*/
      {
	 j=0;
	 while(j<i)
	 {
	    first_cmdline[j] = cmdline[j];
	    j++;
	 }
	 mode = 0;
	 break;
      }
      i++;
   }
   i++;
   int x=0;
   while(i<cnt)
	 {
	    second_cmdline[x] = cmdline[i];
	    x++;
	    i++;
	 }


   first_cmdline[j]=NULL;
   second_cmdline[x]=NULL;

   if(mode == 0)
   {
      int p[2];
      pipe(p);

      int pid = fork();
      if (pid==0) {
	 close(p[0]);
	 dup2(p[1], 1);
	 close(p[1]);
	 _system(first_cmdline);
	 return 0;
      }
      wait(&pid);
      pid = fork();
      if (pid == 0) 
      {
      	 close(p[1]);
	 dup2(p[0], 0);
	 close(p[0]);
	 _system(second_cmdline);
	 return 0;
      }
      close(p[0]);
      close(p[1]);
      wait(&pid);


      return 1;
     
   }

   return 1;
   
}

void my_handler(int signum)
{
   if (signum == SIGINT)
   {
      printf("\nA SIGINT signal!\n");
   }
   else if(signum == SIGQUIT)
      printf("\nA SIGQUIt signal!\n");
}
