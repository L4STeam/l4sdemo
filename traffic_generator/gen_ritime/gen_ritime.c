#include <math.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

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
/* This function returns a negative exponential delay based on
   on mean interarrival time
*/
double get_negexp_time(int mean_interarrival_time){

    double t = log( drand48() ) * -1.0;
    t *= mean_interarrival_time;
    return t;
}

int main(int argc, char *argv[]){
    
    if(argc != 3){
	fprintf(stderr, 
		"Usage: %s <Average client request interarrival time (usec)> <nr of random samples>\n", argv[0]);
	
	exit(1);
    }
    
    double mean_interarrival_time = atof(argv[1]);
    int nr_samples = atoi(argv[2]);

    /* HTTP request interarrival time */
    int http_request_itime;

    /* Seed to random function based on process PID */
    srand48(getpid());

    int i, j;	
    for (i = 0; i < nr_samples; ++i) {
	char filename[35] = "rit";
    	char id[5];
	char mit[15];
    	itoa(i, id);
	itoa(mean_interarrival_time, mit);
	strcat(filename, mit);
	strcat(filename, "_");
    	strcat(filename, id);
    	strcat(filename, ".txt");
    	FILE *f_out = fopen(filename, "w");
    	if (f_out == NULL) {
        	printf("Error opening output file!\n");
        	exit(1);
   	 }
	for (j = 0; j < 100000; ++j) {
		
      		//Calculate HTTP request interarrival time
     	 	http_request_itime = (int) get_negexp_time(mean_interarrival_time);
		fprintf(f_out, "%d\n", http_request_itime);
	}
	fclose(f_out);
      }	
      return 0;    
}
