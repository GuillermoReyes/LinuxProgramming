#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <getopt.h>
#include <sys/stat.h>
#include <fcntl.h>
/* The ta's reccomended using dup() rather than dup2*/
static int segfault_flag; 
static int catch_flag;
void read_and_write(){
  char buff[512]; 
  while(read(0,&buff,1)>0){
    //if(buff == EOF)
    //  break; 
    //else
      write(1, &buff, 1);
    
  }
  exit(0); 
  
}
void sig_handler(int sig){
  if(sig == SIGSEGV){
    fprintf(stderr, "Caught a segmentation fault\n");
    exit(3);
  }

}
void seg_fault(){
  char* pointer = NULL; 
  //dwe're about to do something dumb
  pointer[10] = 'a'; 
  

}
int main(int argc, char* argv[]){
  //if --signal , then we register the signhandler
  // if segfault, do a segfault
  // then we pass in the argyuments, the defaults for stdin & stdout are
  //       0, and 1, 
  // 
  int c; 
  static struct option long_options[] = {
    {"input", required_argument, 0, 'i'},
    {"output", required_argument, 0, 'o'},
    {"segfault", no_argument, &segfault_flag, 1},
    {"catch", no_argument, &catch_flag, 1},
    {0,0,0,0}

  };
  while(1){
    int option_index;
    c = getopt_long (argc, argv, "i:o:", 
		     long_options, &option_index);
    /* detect end of the options */
    if (c == -1){
      break;
    }
    switch(c){
    case 0:
      /* some opetions return a 0 */
      break;
    case 'i': {
      int fd0 = open(optarg, O_RDONLY);
      if(fd0 == -1){
	fprintf(stderr, "Error in opening file\n");
	perror("error");
	exit(1);

      }
      close(0);
      dup(fd0);
      close(fd0);
      break;
    }
    case 'o': { 
      int out = open(optarg,O_WRONLY);
      if(out == -1){
	fprintf(stderr,"Error in opening output file");
	perror("Error");
	exit(2);
      }
      //dup2(out, 1);
      close(1);
      dup(out);
      close(out);
      break; 
    }
    default:
      exit(-1);; 

    }
  }
  if(catch_flag){
    //TODO: implemet a catch routine
    if(signal(SIGSEGV, sig_handler)==SIG_ERR)
      printf("can't catch error\n");
  }
  if(segfault_flag){
    //TODO: create a segfault
    seg_fault();
  }
  read_and_write();
  return 0;
}
