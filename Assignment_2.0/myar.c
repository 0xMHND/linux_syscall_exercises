//
//  main.c
//  myar
//
//  Created by Muhannad Alarifi on 7/10/15.
//  Copyright (c) 2015 Muhannad Alarifi. All rights reserved.
//

#define _POSIX_C_SOURCE 200809L

/* Standard libraries */
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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


int _remove(char * path)
{
  int err = unlink(path);
  if(err == -1)
    {
      perror("Couldn't unlink the file, maybe we don't have permission.");
      return -1;
    }
  return 0;
}
/* Functins Declaration. */

void init_array(char *array,int cnt);
void append_named_files(int cnt, char *ar_name, char* file_args[]);
int append_all(int cnt, char* ar_name);
int extract(char* ar_name, char* file_name);
void extract_lines(int i, int j, FILE *ar_fp ,char *file_name);
void delete_lines(int i, int j, FILE *ar_fp,char *file_name);
int test2(char *ar_name, char *file_name);
int find_line(FILE *ar_fp, char *name, int prv_line, int mode);
int delete(char *ar_name, char *file_name);

int main(int argc, char** argv)
{
  
  char* file_value = NULL;
  char *ar_name = NULL;
  struct ar_hdr file[argc];
  int cnt = 0;
  int j,i=0;  
  int current;
    
  // Set the getopt's opterr to a known (no error) value.
  opterr = 0;
    
  // Provide the user with consistent help
    char* help_str = "Usage: \n"
    "myar -[qxtvdAw] Archive_name file_name ....  \n"
    "\n"
    "-q\tquickly append named files to archive\n"
    "-x\textract named files\n"
    "-t\tprint a concise table of contents of the archive\n"
    "-v\tiff specified with -t, print a vebose table of contents of the archive\n"
    "-d\tdelete named files from the archive\n"
    "-A\tquickly append all 'reguler' files in the current directory (except itself)\n"
    "-w\tfor a timeout, add all modified files to the archive (except itself)\n";
    
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
       fprintf(stderr, help_str);
       return 1;
    }

    for(j=0; i<argc; j++,i++)
    {
       strcpy(file[j].ar_name,argv[i]);
       cnt++;
    }
    j=0;
    while( (current = getopt(argc, argv, "qxtvdAw")) != -1 )
      {
        //printf("Parsing argument: %c\n", current);
        switch(current)
	  {
	  case 'q': // Append named files to archive.
	    append_named_files(cnt,ar_name, &file);
	    break;
	  case 'x': // extract named files.
	    while(j<cnt)
	    {
	       extract(ar_name, &file[j]);
	       j++;
	    }
	    break;
	  case 't': // print a concise contents table of the archive.
	    break;
	  case 'v': // iff specified with -t,print a verbose contents table of the archive.
	    break;
	  case 'd': // delete named files from archive.
	    j=0;
	    while(j<cnt)
	    {
	       delete(ar_name, &file[j]);
	       j++;
	    }
	    break;
	  case 'A': // append all "reguler" files in the current dir.
	    append_all(cnt,ar_name);
	    break;
	  case 'w': // for a given timeout, add all modified files to the archive.
	    break; 
	  default:
	    fprintf(stderr, help_str);
	    return 1;
	  }
      }
    printf("myar: creating %s\n",ar_name);
    return 0;
}

void append_named_files(int cnt, char *ar_name, struct ar_hdr file[])
{	
	FILE *ar_file = fopen(ar_name, "a");
   	fseek(ar_file,0,SEEK_END);	// Check if file is empty
       	if(ftell(ar_file)==0)
	   fputs(ARMAG, ar_file);
	int i=0;
	for( i=0; i<(cnt); i++)
	{	
	   struct stat st;
	   int ret = stat(file[i].ar_name, &st);
	   fprintf(ar_file, "%-16s%-12ld%-6ld%-6ld%-8o%-10ld%-2s",file[i].ar_name, st.st_mtime, st.st_uid, st.st_gid, st.st_mode, st.st_size, ARFMAG);
	   int nbytes = 0;
	   char buff[BSIZE];
	   int file_fd = open(file[i].ar_name, read_options);
	   //printf("Oh dear, something went wrong with read()! %s\n", strerror(errno));
	   while((nbytes = read(file_fd, buff, BSIZE)) > 0)
	   {
	     if(fwrite(buff, 1,nbytes,ar_file) != nbytes)
	       fprintf(stderr, "\nOops, couldn't write what was read.\n");
	   }
	   close(file_fd);
	}
		
	fclose(ar_file);
} 
/* Function name: Append_all
 * Usage: Append all regular file in the current directory except the the archive itself, main or main.c
 * */
int append_all(int cnt, char* ar_name)
{
   DIR *dir;
   struct dirent *dp;
   struct stat st;
   int ret;
   char * file_name;
   dir = opendir("."); //current directory
   FILE *ar_file = fopen(ar_name, "a");
   
   fseek(ar_file,0,SEEK_END);	// Check if file is empty
   if(ftell(ar_file)==0)
   {   
      fputs(ARMAG, ar_file);
   }
   
   while ((dp=readdir(dir)) != NULL) //loop through directory
   {
      file_name = dp->d_name; // use it
      ret = stat(file_name,&st);
      /*check 1)Is it a file. 2)don't archive main or main.o or the archive itself.*/
      if(S_ISREG(st.st_mode)&&(strcmp(file_name,"main")!=0)&&(strcmp(file_name,"main.c")!=0)&&(strcmp(file_name,ar_name)!=0))
      {
	   fprintf(ar_file, "%-16s%-12ld%-6ld%-6ld%-8o%-10ld%-2s",file_name, st.st_mtime, st.st_uid, st.st_gid, st.st_mode, st.st_size, ARFMAG);
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

   fclose(ar_file);
   closedir(dir);
   return 0;

}


int extract(char*ar_name,struct ar_hdr file)
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
   init_line = find_line(fp,file.ar_name,0,1);
   end_line = find_line(fp,ARFMAG,init_line,2);
   extract_lines(init_line,end_line,fp,file.ar_name);
   fclose(fp);

   return(0);
}

int delete(char *ar_name, struct ar_hdr file)
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
   init_line = find_line(fp,file.ar_name,0,1);
   end_line = find_line(fp,ARFMAG,init_line,2);
   _remove(ar_name);
   delete_lines(init_line,end_line,fp,ar_name);
   fclose(fp);

   return(0);
   
}
void delete_lines(int i, int j, FILE *ar_fp,struct ar_hdr file)
{
   char str[60];
   int line = 1;
   char buff[BSIZE];
   FILE *file_name = fopen(file.ar_name,"ab+");
   rewind(ar_fp);
   while( fgets (str, 60, ar_fp)!=NULL ) 
   {
      if( (line<(i))||(line>=(j)))
      {	 
	 fprintf(file_name,str);
      }
      line++;
   }
   fclose(file_name);
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
	    if((strcmp(str,name)==0) && line>prv_line)
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
	    if( (strcmp(str,name)==0) && line>prv_line) 
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
void extract_lines(int i, int j, FILE *ar_fp ,char *file_name)
{
   char str[60];
   int line = 1;
   char buff[BSIZE];
   FILE *file_args = fopen(file_name,"ab+");
   rewind(ar_fp);
   while( fgets (str, 60, ar_fp)!=NULL ) 
   {
      if( (line>=(i))&&(line<(j)))
      {	 
	 fprintf(file_args,str);
      }
      line++;
   }
   fclose(file_args);
}

void init_array(char *array,int cnt)
{
   int i =0;
	for(i=0;i<cnt;i++)
	   array[i]='a';
}

int test2(char *ar_name, char *file_name)
{
      char content[80];
      char newcontent[80];

      //Step 1: Open text files and check that they open//
      FILE *fp1, *fp2;
      fp1 = fopen(ar_name,"r");
      fp2 = fopen(file_name,"w");

      if(fp1 == NULL || fp2 == NULL)
      {
	 printf("Error reading file\n");
	 exit(0);
      }
      printf("Files open correctly\n");
      
      //Step 2: Get text from original file//
      while(fgets(content, sizeof(content), fp1) !=NULL)
      {
	 //fputs (content, stdout);
	 strcpy (newcontent, content);
      }

      printf("Text retrieved from original file\n");
      rewind(fp1);
      //Step 3: Copy text to new file//
      while(fgets(content, sizeof(content), fp1) !=NULL)
      {
	 fprintf(fp2, newcontent);
      }
      printf("file created and text copied to it\n");
      
      //Step 4: Close both files and end program// 
      fclose(fp1);
      fclose(fp2);
      return 0;

}
