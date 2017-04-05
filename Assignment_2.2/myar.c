#define _POSIX_C_SOURCE 200809L

/* Standard libraries */
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

/* System call prototypes! */
#include <unistd.h>

/* File information */
#include <sys/stat.h>

/* File Control */
#include <fcntl.h>

/* errno, and error constants */
#include <errno.h>

/*The ar.h header*/
#include <ar.h>

/*Directory search*/
#include <dirent.h>

/* Default buffer size*/
#define BSIZE 128

int read_options  = O_RDONLY | O_CREAT;
int write_options = O_RDWR | O_APPEND | O_SYNC;

/*__********************************************************************
************************************************************************
******************    * Functuins Decleration *   **********************
************************************************************************
************************************************************************
********************************************************************__*/

int _remove(char *path)
{
  int err = unlink(path);
  if(err == -1)
    {
      perror("Couldn't unlink the file, maybe we don't have permission.");
      return -1;
    }
  return 0;
}

/*Major Functions*/
void append_named_files(int cnt, char *ar_name, char* file_args[]);
void extract_named_files(int cnt, char *ar_name, char* file_args[]);
void delete_named_files(int cnt, char *ar_name, char* file_args[]);/*Not wroking*/
int append_all(int cnt, char* ar_name);
int concise_table(char* ar_name);
int verbose_table(char* ar_name);
int delete(char *ar_name, char *file_name);
int find_line(FILE *ar_fp, char *name, int prv_line, int mode);
void delete_lines(int i, int j, FILE *ar_fp,char *file_name);

/*Supporting Functions*/
const char *_help_str();

/*__********************************************************************
************************************************************************
************                    Main                       *************
************************************************************************
************************************************************************
********************************************************************__*/

int main(int argc, char** argv)
{
    
    char *ar_name = NULL;
    char *files_names[argc];
      
    int cnt = 0;
    int j,i=0;
    int current;
    /*---------------------------------------------------*/
    /*---------------------------------------------------*/

    /*---------------------------------------------------*/
    /*---------------------------------------------------*/
    
    /*get archive file name. */
    for (i= 1; i<argc; i++)
      {
	if (argv[i][0] != '-')
	  {
	    ar_name = argv[i];
	    i++;
	    break;
	  }
      }
    
    if(ar_name == NULL)
      {
	fprintf(stderr, _help_str());
	return 1;
      }
    int lgth=strlen(ar_name);
    if( (ar_name[lgth-1]!='a') && (ar_name[lgth-12]!='.') )
      {
	printf("Error: entered invalid archive file name.\npls enter in the form<filename.a>\n");
	exit(0);
      }
    /*Get Arguments*/
    for(j=0; i<argc; j++,i++)
      {
	files_names[j] = argv[i];
	cnt++;
      }
    
    /*---------------------------------------------------*/
    /*---------------------------------------------------*/
    char v;
    j=0;
    while( (current = getopt(argc, argv, "qxtvdAw")) != -1 )
      {
	/*printf("Parsing argument: %c\n", current);*/
	switch(current)
	  {
	  case 'q': /*Append named files to archive.*/
	    append_named_files(cnt,ar_name, files_names);
	    printf("myar: creating %s\n",ar_name);
	    break;
		
	  case 'x': /* extract named files.*/
	    extract_named_files(cnt,ar_name, files_names);
	    break;
		
	  case 't': /* print a concise contents table of the archive.*/
	    concise_table(ar_name);
	    break;
		
	  case 'v': /* iff specified with -t,print a verbose contents table of the archive.*/
	    verbose_table(ar_name);
	    break;
		
	  case 'd': /* delete named files from archive.*/
	    //delete_named_files(cnt,ar_name,files_names);
	    for(j=0;j<cnt;j++)
	    {
		delete(ar_name,files_names[j]);
	    }
	    break;
		
	  case 'A': /* append all "reguler" files in the current dir.*/
	    append_all(cnt,ar_name);
	    printf("myar: creating %s\n",ar_name);
	    break;
		
	  case 'w': /* for a given timeout, add all modified files to the archive.*/
	    break;
		
	  default:
	    fprintf(stderr, _help_str());
	    return 1;
	  }
      }
    
    /*---------------------------------------------------*/
    /*---------------------------------------------------*/
    
    return 0;
}

/*__********************************************************************
************************************************************************
******************    * Functuins Defintions *   ***********************
************************************************************************
************************************************************************
********************************************************************__*/



/**************************************************************************
 **************  * Function name: append_named_files *   ******************
 * Usage: Append named files.
 **************************************************************************
 **************************************************************************/
void append_named_files(int cnt, char *ar_name, char* file_args[])
{
   char new_name[BSIZE];
   FILE *ar_file = fopen(ar_name, "a+");
   fseek(ar_file,0,SEEK_END);	// Check if file is empty
   if(ftell(ar_file)==0)
      fputs(ARMAG, ar_file);
   int i=0;
   for( i=0; i<(cnt); i++)
   {	
      struct stat st;
      int ret = stat(file_args[i], &st);
      strcpy(new_name,file_args[i]);
      strcat(new_name,"/");
      fprintf(ar_file, "%-16s%-12ld%-6lu%-6lu%-8o%-10ld%-2s\n",new_name, st.st_mtime, st.st_uid, st.st_gid, st.st_mode, st.st_size,ARFMAG);
      int nbytes = 0;
      char buff[BSIZE];
      int file_args_fd = open(file_args[i], read_options);
      //printf("Oh dear, something went wrong with read()! %s\n", strerror(errno));
      	   while((nbytes = read(file_args_fd, buff, BSIZE)) > 0)     	  
	   {
	      if(fwrite(buff, 1,nbytes,ar_file) != nbytes)
		 fprintf(stderr, "\nOops, couldn't write what was read.\n");
	   }

	   close(file_args_fd);
   }
   chmod(ar_name,0666);
   fclose(ar_file);
} 
/* *********************************************************************
************  * Function name: append_all *   **************************
* Usage: Append all regular file in the current directory except the
*        the archive itself, main or main.c.
************************************************************************
************************************************************************/
int append_all(int cnt, char* ar_name)
{
  DIR *dir;
  struct dirent *dp;
  struct stat st;
  int ret;
  char *file_name;
  char new_name[BSIZE];
  dir = opendir("."); /*current directory*/
  FILE *ar_file = fopen(ar_name, "a+");
    
  fseek(ar_file,0,SEEK_END);/* Check if file is empty*/
  if(ftell(ar_file)==0)
    {
      fputs(ARMAG, ar_file);
    }
   while ((dp=readdir(dir)) != NULL) /*loop through directory*/
     {
       file_name = dp->d_name;
       ret = stat(file_name,&st);
       /*check 1)Is it a file. 2)don't archive main or main.o or the archive itself.*/
       if(S_ISREG(st.st_mode)&&(strcmp(file_name,"myar")!=0)&&(strcmp(file_name,"myar.c")!=0)&&(strcmp(file_name,ar_name)!=0))
	 {
	   strcpy(new_name,file_name);
	   strcat(new_name,"/");
	   fprintf(ar_file, "%-16s%-12ld%-6u%-6u%-8o%-10lld%-2s\n",new_name, st.st_mtime, st.st_uid, st.st_gid, st.st_mode, st.st_size,ARFMAG);
	   int nbytes = 0;
	   char buff[BSIZE];
	   int file_args_fd = open(file_name, read_options);
	   while((nbytes = read(file_args_fd, buff, BSIZE)) > 0)
	     {
	       if(fwrite(buff, 1,nbytes,ar_file) != nbytes)
		 fprintf(stderr, "\nOops, couldn't write what was read.\n");
	     }
	   close(file_args_fd);
	 }
     }
   chmod(ar_name, 0666);
   fclose(ar_file);
   closedir(dir);
   return 0;
}

/* *********************************************************************
************  * Function name: concise_table *   **************************
* Usage: print the names of files in the archive.  
************************************************************************
************************************************************************/

int concise_table(char* ar_name)
{
   char str[BSIZE],file_name[BSIZE],temp[BSIZE];
   long mtime, uid, gid, mode, size;
   int cnt=1;
   FILE *ar_file = fopen(ar_name,"r");

   while (!feof(ar_file))
   {
      while( fgets(str,sizeof(str),ar_file) !=NULL )
      {	 
	 if(strlen(str)>59)
	 {
	    if( (str[58]== '`') && (str[59]== '\n'))
	    {
	       strcpy(file_name,str);
	       char *p = strchr(file_name, '/');
	       if (!p) /* deal with error: / not present" */
		  printf("smthing is wrong in locating '/'\n");
	       *p = 0;
	       
	       strncpy(temp,str+16,12);
	       p = strchr(temp, ' ');
	       if (!p) /* deal with error: / not present" */
		  printf("smthing is wrong in locating '/'\n");
	       *p = 0;
	       mtime = strtol(temp,NULL,10);
	      
	       strncpy(temp,str+28,6);
	       p = strchr(temp, ' ');
	       if (!p)
		  printf("smthing is wrong in locating '/'\n");
	       *p = 0;
	       uid = strtol(temp,NULL,10);
	   
	       strncpy(temp,str+34,6);
	       p = strchr(temp, ' ');
	       if (!p)
		  printf("smthing is wrong in locating '/'\n");
	       *p = 0;
	       gid = strtol(temp,NULL,10);

	       strncpy(temp,str+40,8);
	       p = strchr(temp, ' ');
	       if (!p)
		  printf("smthing is wrong in locating '/'\n");
	       *p = 0;
	       mode = strtol(temp,NULL,8);
	       
	       strncpy(temp,str+48,10);
	       p = strchr(temp, ' ');
	       if (!p)
		  printf("smthing is wrong in locating '/'\n");
	       *p = 0;
	       size = strtol(temp,NULL,10);
	 
	       printf("%-16s\n",file_name);
	    }
	 }
      }

   }
}
/**************************************************************************
 **************  * Function name: extract_named_files *   ******************
 * Usage: Extract named files.
 **************************************************************************
 **************************************************************************/
void extract_named_files(int cnt, char *ar_name, char* file_args[])
{
   char new_name[BSIZE],str[BSIZE], temp[BSIZE];
   FILE *ar_file = fopen(ar_name, "r");
   
   int i=0,line=0;
   for( i=0; i<(cnt); i++)
   {	
      strcpy(new_name,file_args[i]);
      rewind(ar_file);
      FILE *new_file = fopen(file_args[i],"w");
     // FILE *new_file = fopen("newfile.txt","a+");
      long mtime, uid, gid, mode, size;
      int onetwo=0,x=0;
      while (!feof(ar_file))
      {
	 while( fgets(str,sizeof(str),ar_file) !=NULL )
	 {
	   line++; 
	    if(strlen(str)>59)
	    {
	       if( (str[58]== '`') && (str[59]== '\n'))
	       {
		  strcpy(new_name,str);
		  char *p = strchr(new_name, '/');
		  *p = 0;

		  if( (x=strcmp(new_name,file_args[i])!=0) && onetwo==0 )
		     break;
		  onetwo++;
//		  printf("found header .. %s\t%d\n",new_name,x);
		  strncpy(temp,str+16,12);
		  p = strchr(temp, ' ');
		  *p = 0;
		  mtime = strtol(temp,NULL,10);
		 
		  strncpy(temp,str+28,6);
		  p = strchr(temp, ' ');
		  *p = 0;
		  uid = strtol(temp,NULL,10);
	      
		  strncpy(temp,str+34,6);
		  p = strchr(temp, ' ');
		  *p = 0;
		  gid = strtol(temp,NULL,10);

		  strncpy(temp,str+40,8);
		  p = strchr(temp, ' ');
		  *p = 0;
		  mode = strtol(temp,NULL,8);
		  
		  strncpy(temp,str+48,10);
		  p = strchr(temp, ' ');
		  *p = 0;
		  size = strtol(temp,NULL,10);

		 break; 
	       }
	    }
	    if(onetwo==2)
		break;
	    if(onetwo==1)
	    {
//	       printf("copying line %d\n",line);
	       fwrite(str, 1,strlen(str),new_file);
   	    }
	 }
	 if(onetwo==2)
	    break;
      }
      chmod(file_args[i],mode);
      fclose(new_file);
   } 
}
/**************************************************************************
 **************  * Function name: delete_named_files *   ******************
 * Usage: delete named files.
 **************************************************************************
 **************************************************************************/
void delete_named_files(int cnt, char *ar_name, char* file_args[])
{
   char new_name[BSIZE],str[BSIZE], temp[BSIZE];
   struct stat st;
  // stat(ar_name,&st);
  // FILE *ar_file = fopen(ar_name, "r");
  // _remove(ar_name);
  // FILE *new_ar_file = fopen(ar_name, "r");
   int i=0,line=0;
   for( i=0; i<(cnt); i++)
   {

      stat(ar_name,&st);
      FILE *ar_file = fopen(ar_name, "r");
      _remove(ar_name);
      FILE *new_ar_file = fopen(ar_name, "w");
      strcpy(new_name,file_args[i]);
      rewind(ar_file);
      int onetwo=0,x=0;
      printf("\n%s\n",new_name);
      while (!feof(ar_file))
      {
	 while( fgets(str,sizeof(str),ar_file) !=NULL )
	 {
	    printf("got new line\n");
	   line++; 
	   if(strlen(str)>59)
	    {
	       if( (str[58]== '`') && (str[59]== '\n'))
	       {
		  strcpy(new_name,str);
		  char *p = strchr(new_name, '/');
		  *p = 0;
		  
		 printf("inside header..\n");
		  if(onetwo==1)
		  {  
		    printf("CHange onetwo to 0\n"); 
		     onetwo==0;
		  }
//		  for(x=0; x<cnt; x++)
//		  {
		     if( (strcmp(new_name,file_args[i])==0) && onetwo==0 )
		     {
			printf("onetwo: %d \nfound targeted heaeder:\n%s == %s\n",onetwo,new_name,file_args[x]);
			onetwo=1;
			break;
		     }
	  	  }
	 	   
	       }
	    }
	
	    if(onetwo==0)
	    {
	       printf("copy.. \n");
	       fwrite(str, 1,strlen(str),new_ar_file);
   	    }
	 }
      
      chmod(ar_name,st.st_mode);
      fclose(ar_file);
      fclose(new_ar_file);
      } 
}
/* *********************************************************************
************  * Function name: verbose_table *   **************************
* Usage: print detailed table of contents.
************************************************************************
************************************************************************/

int verbose_table(char* ar_name)
{
   char str[BSIZE],file_name[BSIZE],temp[BSIZE];
   long mtime, uid, gid, mode, size;
   int cnt=1;
   FILE *ar_file = fopen(ar_name,"r");

   while (!feof(ar_file))
   {
      while( fgets(str,sizeof(str),ar_file) !=NULL )
      {	 
	 if(strlen(str)>59)
	 {
	    if( (str[58]== '`') && (str[59]== '\n'))
	    {
	       strcpy(file_name,str);
	       char *p = strchr(file_name, '/');
	       if (!p) /* deal with error: / not present" */
		  printf("smthing is wrong in locating '/'\n");
	       *p = 0;
	       
	       strncpy(temp,str+16,12);
	       p = strchr(temp, ' ');
	       if (!p) /* deal with error: / not present" */
		  printf("smthing is wrong in locating '/'\n");
	       *p = 0;
	       mtime = strtol(temp,NULL,10);

	       struct tm lt;
 	       localtime_r(&mtime, &lt);
 	       char timbuf[80];
 	       strftime(timbuf, sizeof(timbuf), "%c", &lt);
	       //sprintf(timebuf, "%d_%d.%d.%d_%d:%d:%d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

	       strncpy(temp,str+28,6);
	       p = strchr(temp, ' ');
	       if (!p)
		  printf("smthing is wrong in locating '/'\n");
	       *p = 0;
	       uid = strtol(temp,NULL,10);
	       
	       struct passwd *pws;
	       pws = getpwuid(uid);

	       strncpy(temp,str+34,6);
	       p = strchr(temp, ' ');
	       if (!p)
		  printf("smthing is wrong in locating '/'\n");
	       *p = 0;
	       gid = strtol(temp,NULL,10);

	       struct group *g;
	       g = getgrgid(gid);

	       strncpy(temp,str+40,8);
	       p = strchr(temp, ' ');
	       if (!p)
		  printf("smthing is wrong in locating '/'\n");
	       *p = 0;
	       mode = strtol(temp,NULL,8);
	       
	       strncpy(temp,str+48,10);
	       p = strchr(temp, ' ');
	       if (!p)
		  printf("smthing is wrong in locating '/'\n");
	       *p = 0;
	       size = strtol(temp,NULL,10);

	        printf( (S_ISDIR(mode)) ? "d" : "-");
	       	printf( (mode & S_IRUSR) ? "r" : "-");
		printf( (mode & S_IWUSR) ? "w" : "-");
		printf( (mode & S_IXUSR) ? "x" : "-");
		printf( (mode & S_IRGRP) ? "r" : "-");
		printf( (mode & S_IWGRP) ? "w" : "-");
		printf( (mode & S_IXGRP) ? "x" : "-");
		printf( (mode & S_IROTH) ? "r" : "-");
		printf( (mode & S_IWOTH) ? "w" : "-");
		printf( (mode & S_IXOTH) ? "x" : "-");
	       printf(" 1 %-6s  %-6s  %-10ld  %-10s  %-16s\n",pws->pw_name,g->gr_name,size,timbuf+4,file_name);
	    }
	 }
      }
   }
   
}

/*__**********************************************************************
 *************************************************************************
 *******************    * Supporting  Functuins *   **********************
 *************************************************************************
 *************************************************************************
 *********************************************************************__*/

/* Provide the user with consistent help*/

const char *_help_str()
{
   return "Usage: \n"
      "myar -[qxtvdAw] Archive_name file_name ....  \n"
      "\n"
      "-q\tquickly append named files to archive\n"
      "-x\textract named files\n"
      "-t\tprint a concise table of contents of the archive\n"
      "-v\tiff specified with -t, print a vebose table of contents of the archive\n"
      "-d\tdelete named files from the archive\n"
      "-A\tquickly append all 'reguler' files in the current directory (except itself)\n"
      "-w\tfor a timeout, add all modified files to the archive (except itself)\n";
}


int delete(char *ar_name, char *file_name)
{
   FILE *fp;
   int init_line=0;
   int end_line=0;
   /* opening file for reading */
   fp = fopen(ar_name , "r");
   if(fp == NULL) 
   {
      perror("Error opening file");
      return(-1);
   }
   init_line = find_line(fp,file_name,0,1);
   end_line = find_line(fp,"'",init_line,2);
   _remove(ar_name);
   delete_lines(init_line,end_line,fp,ar_name);
   fclose(fp);
   return(0);
				       
}

void delete_lines(int i, int j, FILE *ar_fp,char *file_name)
{
   char str[60];
   int line = 1;
   char buff[BSIZE];
   FILE *file_args = fopen(file_name,"ab+");
   rewind(ar_fp);
   while( fgets (str, 60, ar_fp)!=NULL ) 
   {
      if( (line<(i))||(line>=(j)))
      {	 
	 printf("printing line #%d\n",line);
	 fprintf(file_args,str);
      }
      line++;
   }
   fclose(file_args);
}

int find_line(FILE *ar_fp, char *name, int prv_line, int mode)
{
   char str[BSIZE];
   char c;
   int line =1;
   rewind(ar_fp);
   while( fgets(str,sizeof(str),ar_fp) !=NULL ) 
   {
      switch(mode)
      {
	 case(1):
	    if(strstr(str,name)!=0 && line>prv_line)
	 {
	    return line;
	 }
	    else if( (c=feof(ar_fp)) == EOF)
	    {
	       printf("file not found\n");
	       return line;
	    }
	    line++;
	 case(2):
	    if( ( str[58]=='`' ) && line>prv_line) /* 96 is ascii dec value for ' */
	 {
	    return line;
	 }
	    else if( (c=feof(ar_fp)) == EOF)
	    {
	       return line;
	    }
	    line++;
      }
   }
   printf("ERROR: didn't find your file in archive");
}
