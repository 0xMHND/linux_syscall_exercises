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
char **split_line(char *line, int *cnt);

/*__********************************************************************
************************************************************************
************                    Main                       *************
************************************************************************
************************************************************************
********************************************************************__*/

int main(int argc, char* argv[])
{
   char *cmdline[4];
   int i=1,count;
   
   //_system(cmdline,&count); 
   
   while(i != 0){
      i=loop(cmdline,&count);
   }
   
   printf("Exiting ... \n");
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
   int i=0,cnt; 
  /* Get input */ 
   printf("$> ");
   fgets(line, 100, stdin);
   printf("You entered: %s\n",line);
  
  /* parse input */ 
   args = split_line(line,&cnt);
   
  /* copy parsed line */ 
   while(i<cnt)
   {
      cmdline[i]=args[i];
      i++;
   }
   *count=cnt;

   cmdline[i]=NULL;/* Null terminate at the end */

   if(!strcmp(cmdline[0],"exit"))
     return 0; 

 
   _system(cmdline);
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
   char line[100];
   char **args;
   int i=0,cnt;
   
  /* Get input */ 
//   printf("$> ");
//   fgets(line, 100, stdin);
//   printf("You entered: %s\n",line);
  
  /* parse input */ 
//   args = split_line(line,&cnt);
   
  /* copy parsed line */ 
//   while(i<cnt)
 //  {
   //   cmdline[i]=args[i];
     // i++;
   //}
   //*count=cnt;

   //cmdline[i]=NULL;/* Null terminate at the end */


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
   printf("cmd path: %s\n",path);
   
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

   for(int i=0; i<BSIZE; i++)
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
   
  // buf[0] = getenv("PATH");
   int cnt=0,i=0;
   
   char **tokens = split_line(buf,&cnt);
  // while(i!=cnt)
    //  printf("%s\n",tokens[i++]);
   
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
	       printf("found the command\n");
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


#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \n:"
char **split_line(char *line, int *cnt)
{
  int bufsize = LSH_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token;

  if (!tokens) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, LSH_TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += LSH_TOK_BUFSIZE;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, LSH_TOK_DELIM);
  }
  tokens[position] = NULL;
  *cnt = position;
  return tokens;
}

