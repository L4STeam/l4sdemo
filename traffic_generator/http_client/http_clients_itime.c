/*
   This program starts HTTP clients which will connect to
   a server on a specified port and download files.

   The number of simultanious clients is determined by
   the HTTP request interarrival time. The HTTP request
   intararrival time is calculated from a negative exponential
   function based on a mean interarrival time.

   In the tests, the mean interarrival time was set to
   647000 usec.

*/

#include <math.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/types.h>
#include <assert.h>
#include <time.h>
#include <stropts.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <errno.h>

#include <sys/uio.h>
#include <ctype.h>
#include <sys/time.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 80000
struct ThreadParam {
    pthread_mutex_t data_mutex;
    double *time_send;
    double *size_send;
    double *time_write;
    double *size_write;
    double *time_send_hs;
    double *time_write_hs;

    int cur_index;
};

int maxtpsend = 10000;

void initThreadParam(struct ThreadParam* tp) {
    pthread_mutex_init(&tp->data_mutex, NULL);
    tp->time_send = (double *)malloc(sizeof(double)*maxtpsend);
    tp->size_send = (double *)malloc(sizeof(double)*maxtpsend);
    tp->time_write = (double *)malloc(sizeof(double)*maxtpsend);
    tp->size_write = (double *)malloc(sizeof(double)*maxtpsend);

    tp->time_send_hs = (double *)malloc(sizeof(double)*maxtpsend);
    tp->time_write_hs = (double *)malloc(sizeof(double)*maxtpsend);

    bzero(tp->time_send, sizeof(double)*maxtpsend);
    bzero(tp->size_send, sizeof(double)*maxtpsend);
    bzero(tp->time_write, sizeof(double)*maxtpsend);
    bzero(tp->size_write, sizeof(double)*maxtpsend);
    bzero(tp->time_send_hs, sizeof(double)*maxtpsend);
    bzero(tp->time_write_hs, sizeof(double)*maxtpsend);

    tp->cur_index = 0;
}
void error(char *msg)
{
    perror(msg);
    exit(0);
}


int port; // port to connect to
char *host; // host to connect to
int iat_rate;
char *data_transfer_host;
int data_transfer_port;
int samples[100000]; //Array holding the exponentially distributed interarrival times
int sampleindex;
int active_clients;
pthread_mutex_t mutex;
static FILE *f_wo_hs;
static FILE *f_w_hs;
int data_transfer;

struct ThreadParam tp;

void transfer_compl_data (){
    int sockfd, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buf;
    char sendbuf[BUFFER_SIZE];
    bzero(sendbuf, BUFFER_SIZE);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
        fprintf(stderr, "ERROR opening socket for completion data transfer\n");
        pthread_exit(0);
    }
    server = gethostbyname(data_transfer_host);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host (completion data transfer)\n");
        pthread_exit(0);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(data_transfer_port);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
        error("ERROR connecting to socket (completion data transfer)");
	pthread_exit(0);
    }

	int nr_samples = 0;
	struct timeval start_time,end_time,diff_time;
    do {
         gettimeofday(&start_time, NULL);
         pthread_mutex_lock(&tp.data_mutex);
        if (tp.cur_index > 0){
            nr_samples = tp.cur_index;
            tp.cur_index = 0;
            double *tmp_size = tp.size_write;
            double *tmp_time = tp.time_write;
            double *tmp_time_hs = tp.time_write_hs;

            tp.size_send = tp.size_write;
            tp.time_send = tp.time_write;
            tp.time_send_hs = tp.time_write_hs;
            tp.size_write = tmp_size;
            tp.time_write = tmp_time;
            tp.time_write_hs = tmp_time_hs;

            pthread_mutex_unlock(&tp.data_mutex);

	    int buf_index = 0;
	    int bytes_per_chunk =  sizeof(double)*nr_samples;
	    memcpy(&sendbuf[buf_index],tp.size_send, bytes_per_chunk);
	    buf_index += bytes_per_chunk;
	    memcpy(&sendbuf[buf_index],tp.time_send, bytes_per_chunk);
            buf_index += bytes_per_chunk;
	    memcpy(&sendbuf[buf_index],tp.time_send_hs, bytes_per_chunk);
            buf_index += bytes_per_chunk;
	    n = write(sockfd,(char*)&buf_index, sizeof(int));
	    int bytes_sent = 0;
	    n = 0;
	    while (bytes_sent < buf_index){
		  n = write(sockfd,&sendbuf[bytes_sent], buf_index-bytes_sent);
		  if (n < 0) {
			error("ERROR writing to socket (completion data transfer)");
			pthread_exit(0);
		   }

		  bytes_sent += n;
		  n = 0;
            }
	    bzero(tp.size_send, sizeof(double)*maxtpsend);
            bzero(tp.time_send, sizeof(double)*maxtpsend);
            bzero(tp.time_send_hs, sizeof(double)*maxtpsend);
        } else
		pthread_mutex_unlock(&tp.data_mutex);

        gettimeofday(&end_time, NULL);
        timersub(&end_time,&start_time,&diff_time);
        uint32_t diff_us = diff_time.tv_sec*1000000 + diff_time.tv_usec;
        if (diff_us < 1000000)
            usleep(1000000-diff_us);

    } while (n >= 0);
}

/* This method reads the values of a Pareto distribution sample file
   into an array
 */
void initialize_distribution(char *filename){
  FILE* fd;
  int i = 0, r;

  if (!(fd = fopen(filename, "r"))) {
    perror("Cannot open the distribution file!");
    exit(1);
  }

  for(i=0; i < 100000; i++){
    if (fscanf(fd, "%d", &r) <= 0) {
        perror("error reading samples");
        exit(1);
    }
    // scale to the correct link capacity (reference rate is 40Mbps)
    samples[i] = r * 40 / iat_rate;
  }
  fclose(fd);
}

/* This function returns a negative exponential delay based on
   on mean interarrival time
*/
double get_negexp_time(int mean_interarrival_time){

    double t = log( drand48() ) * -1.0;
    t *= mean_interarrival_time;
    return t;
}


// this function returns a current monotonic timestamp in usec (never going backwards and not affected by DLST and NTP!!!)
int64_t getStamp()
{
	struct timespec monotime;
    clock_gettime(CLOCK_MONOTONIC, &monotime);
    return ((int64_t)monotime.tv_sec) * 1000000 + monotime.tv_nsec / 1000;
}

/* A single client is represented and run by this thread function:

   The thread will connect to the server on the specified port
   and download a file until there is no more to read from
   the socket. Finally, the socket is closed.

*/
void run_client(){

  struct sockaddr_in addr;
  int sock;
  int read_data = 1;
  int received_bytes = 0;
  int r_rcv = 0;

  int err;
  char buffer[5000];

  sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

  /* Make socket reusable */
  int reuse = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

  memset(&addr, 0, sizeof(struct sockaddr_in));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = inet_addr(host);

  int64_t hs_start, transfer_start, transfer_end, transfer_time, transfer_time_w_hs;

  /* Connect to host */
  hs_start = getStamp();
  if((err = connect(sock, (struct sockaddr *)&addr, sizeof addr)) < 0){
    perror("error connecting to server, thread exiting");
    close(sock);
    pthread_mutex_lock(&mutex);
    active_clients--;
    pthread_mutex_unlock(&mutex);

    pthread_detach(pthread_self());
    pthread_exit(0);

}

  /* Read data from socket until there is no more to read */

  transfer_start = getStamp();
  while(read_data){
    err = recv(sock, buffer, 5000, 0);
    if(err <= 0){
      if( err == -1 || err == 0 )
      {
        if( err == 0 ||
        errno == EBADF ||
        errno == ENOTCONN ||
        errno == ENOTSOCK )
        {
          read_data = 0;
        }
        else if( errno == EAGAIN ||
                 errno == EINTR ||
                 errno == EWOULDBLOCK )
        {
          continue;
        }
        else
        {
          perror( "failed reading from socket" );
          read_data = 0;
        }
      }
    }
    else /* to avoid the negative error codes are added to the size */
      received_bytes += err;
  }
  transfer_end = getStamp();
  transfer_time = transfer_end - transfer_start;
  transfer_time_w_hs = transfer_end - hs_start;
  // Decrease the number of active clients and close socket.
  pthread_mutex_lock(&mutex);
  active_clients--;
  if (received_bytes > 0) {
 	fprintf(f_wo_hs, "%d %d\n", received_bytes, (uint32_t)transfer_time);
  	fprintf(f_w_hs, "%d %d\n", received_bytes, (uint32_t)transfer_time_w_hs);
  }
  pthread_mutex_unlock(&mutex);
  close(sock);
  if (data_transfer)
  {
    pthread_mutex_lock(&tp.data_mutex);
    if (tp.cur_index < maxtpsend) { // prevent crash on overload of this buffer
      tp.size_write[tp.cur_index] = (double) received_bytes;
      tp.time_write[tp.cur_index] = (double) transfer_time;
      tp.time_write_hs[tp.cur_index] = (double) transfer_time_w_hs;
      tp.cur_index++;
    }
    pthread_mutex_unlock(&tp.data_mutex);
  }

  // Detach thread from memory and exit
  pthread_detach(pthread_self());
  pthread_exit(0);

}

/* This function is responsible for running clients */

int run_httpclients(){

  int err;
  pthread_t thread_client;
  int i = 0;

  /* HTTP request interarrival time */
  int http_request_itime;

  /* Seed to random function based on process PID */
  // srand48(getpid());
  active_clients = 0;

  printf("Start client \n");

  int64_t curr_stamp, http_request_stamp;

  http_request_stamp = getStamp();

  while(1){

    curr_stamp = getStamp();

    //Get HTTP request interarrival time
    http_request_stamp += samples[sampleindex++];
    if (sampleindex >= 100000)
	sampleindex = 0;
    //Sleep before doing a new HTTP request
    while (http_request_stamp - curr_stamp > 0) // to mitigate wrapping
    {
        usleep(http_request_stamp - curr_stamp);
      curr_stamp = getStamp();
    }
    //printf("sleep for %d\n", http_request_itime);

    if((err = pthread_create( &(thread_client), NULL,
              (void*)run_client, NULL)) != 0){
      perror("pthread_create");
    }

    i++;
    pthread_mutex_lock(&mutex);
    active_clients++;
    pthread_mutex_unlock(&mutex);

    // Slow down output to screen
    if(i%250 == 0){
      i = 0;
      printf("[Clients on port: %d] Active clients: %d\n", port, active_clients);
    }
  }
}

int main(int argc, char *argv[]){

  if(argc < 4 || argc > 7){
    fprintf(stderr,
        "Usage: %s <HTTP Server address> <HTTP Server port> <client request interarrival time file> (optional:) <iat_rate> (optional:) <Data transfer server> <Data transfer port>\n", argv[0]);

    exit(1);
  }

  host = argv[1];
  port = atoi(argv[2]);
  char *filename_interarrival_time = argv[3];
  int nextarg = 4;
  if (argc == 5 || argc == 7) {
    iat_rate = atoi(argv[nextarg++]);
  }
  else
    iat_rate = 40;
  if (argc == 6 || argc == 7) {
    data_transfer_host = argv[nextarg++];
    data_transfer_port = atoi(argv[nextarg++]);
    int err;
    pthread_t thread_client;
    data_transfer = 1;
    initThreadParam(&tp);
    if((err = pthread_create(&thread_client, NULL, (void*)transfer_compl_data, NULL)) != 0){
        perror("pthread_create for data transfer thread");
    }
  } else
    data_transfer = 0;
  initialize_distribution(filename_interarrival_time);
  sampleindex = 0;
  pthread_mutex_init(&mutex, NULL);
  char filename[50] = "completion_time_port";
  strcat(filename, argv[2]);
  if (!(f_wo_hs = fopen(filename, "w"))) {
    printf("Error opening output file!\n");
    exit(1);
  }

  strcat(filename, "_w_hs");
  if (!(f_w_hs = fopen(filename, "w"))) {
    printf("Error opening output file!\n");
    exit(1);
  }

  setlinebuf(f_wo_hs);
  setlinebuf(f_w_hs);
  run_httpclients();

}
