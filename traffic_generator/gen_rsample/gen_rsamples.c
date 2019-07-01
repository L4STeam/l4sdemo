#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
//#include <linux/types.h>
#include <assert.h>
#include <sys/time.h>
//#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <sys/uio.h>
#include <errno.h>
#include <strings.h>

int samples[100000]; //Array holding the Pareto distribution samples file
int sampleindex;

/* reverse:  reverse string s in place */
 void reverse(char s[])
 {
     int i, j;
     char c;

     for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
         c = s[i];
         s[i] = s[j];
         s[j] = c;
     }
 }

 /* itoa:  convert n to characters in s */
 void itoa(int n, char s[])
 {
     int i, sign;

     if ((sign = n) < 0)  /* record sign */
         n = -n;          /* make n positive */
     i = 0;
     do {       /* generate digits in reverse order */
         s[i++] = n % 10 + '0';   /* get next digit */
     } while ((n /= 10) > 0);     /* delete it */
     if (sign < 0)
         s[i++] = '-';
     s[i] = '\0';
     reverse(s);
 }

/* This method reads the values of a Pareto distribution sample file
   into an array
 */
void initialize_distribution(){
  FILE* fd;
  int i = 0;

  if (!(fd = fopen("samples.txt", "r"))) {
    perror("open");
    exit(1);
  }

  for(i=0; i < 100000; i++){
    if (fscanf(fd, "%d", &samples[i]) <= 0) {
	perror("error reading samples");
	exit(1);
    }
  }
  fclose(fd);
}

void writeRandomSample(int sampleid)
{
    struct timeval randomize;
    long seed;

    gettimeofday(&randomize, NULL);
    seed = (&randomize)->tv_usec;
    srand((int)seed);

    char filename[25] = "rs";
    char id[5];
    itoa(sampleid, id);
    strcat(filename, id);
    strcat(filename, ".txt");
    FILE *f_out = fopen(filename, "w");
    if (f_out == NULL) {
        printf("Error opening output file!\n");
        exit(1);
    }
    for (int i = 0; i < 100000; ++i)
    {
	fprintf(f_out, "%d\n", samples[(rand() % 100000)]);
    }
    fclose(f_out);
    //return samples[(rand() % 100000)];
}

int main(int argc, char *argv[]){

  sampleindex = 0;

  if(argc != 2){
    fprintf(stderr,
	    "Usage: %s <nr of random samples>\n", argv[0]);
    exit(1);


  }
  int nr_rsamples = atoi(argv[1]);

  initialize_distribution(); //read the the Pareto distribution from file

  for (int i = 0; i < nr_rsamples; ++i)
{
	writeRandomSample(i);
}

  return 0;
}
