#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/time.h>
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
#include <string.h>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/* We currently only check for the following CS codepoints for classifying type of service:
* CS0 = 0x0 (defualt)
* CS1 = 0x8
* CS5 = 0x28
* CS7 = 0x38
* 
* Additionally, a mixed mode (0xff) randomly sets for each flow one of the four CS levels.
*/
#define CS0 0x0
#define CS1 0x20
#define CS5 0xa0
#define CS7 0xe0

static unsigned char tos = CS0;
static unsigned char mode = CS0;

int sockets[100000]; // Array holding the sockets to pass to the thread
int samples[100000]; //Array holding the Pareto distribution samples file size to pass to the thread
char buffer[50000]; // the send buffer, everyone can share as it is read only

/* This method reads the values of a Pareto distribution sample file
   into an array
 */
void initialize_distribution(char *filename){
  FILE* fd;
  int i = 0;

  if (!(fd = fopen(filename, "r"))) {
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


/* This method return the value (file size) from a random location in the Pareto
   distribution array
 */
int get_transfer_size(){

    struct timeval randomize;
    long seed;

    gettimeofday(&randomize, NULL);
    seed = (&randomize)->tv_usec;
    srand((int)seed);

    return samples[(rand() % 100000)];
}

// create a struct or array to send mysampleindex and tos
void send_to_socket(int mysampleindex){

    int cur_sock = sockets[mysampleindex];
    int transfer_size = samples[mysampleindex++];
    int transmit_bytes = transfer_size;
    int bytes_xmt = 0;
    int err;
    int r_snd = 0;
    int j = 0;

    /* set type of service */
    const unsigned char mixed[] = {CS0, CS1, CS5, CS7};
    const int CS_LEVELS = ARRAY_SIZE(mixed);

    if (mode == 0xff)
      tos = mixed[random() % CS_LEVELS];
    else
      tos = mode;
    
    if ((setsockopt(cur_sock, IPPROTO_IP, IP_TOS, &tos, sizeof(tos))) < 0) {
      fprintf(stderr, "failed to set type of service: %s\n", strerror(errno));
    }

    while(bytes_xmt < transfer_size){

      if(transmit_bytes > 50000){
        transmit_bytes = 50000;
      }
      if((err = send(cur_sock, buffer, transmit_bytes, 0)) < 0){

          if( err == -1 || err == 0 )
          {
              if( errno == 0 ||
                  errno == EBADF ||
                  errno == ENOTCONN ||
                  errno == ENOTSOCK )
              {
                  break;
              }
              else if( errno == EAGAIN ||
                       errno == EINTR ||
                       errno == EWOULDBLOCK )
              {
                  continue;
              }
              else
              {
                  perror( "failed writing to socket" );
                  break;
              }
          }

      }

      bytes_xmt += err;
      transmit_bytes = transfer_size - bytes_xmt;
    }

    shutdown(cur_sock, 2); // shuts down the read operations only

    if((err = close(cur_sock)) < 0){
        perror("close");
    }

    pthread_detach(pthread_self());
    pthread_exit(0);
}

/* This method creates and then returns a listen socket for incoming requests
 */

int create_listen_socket(int port){

    int listen_sock;
    struct sockaddr_in serveraddr;

    /* Create a request-socket */
    if ((listen_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("socket");
        exit(1);
    }

    /* Make socket reusable */
    int reuse = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));


    /* Create address struct */
    bzero((void *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = INADDR_ANY;
    serveraddr.sin_port = htons(port);

    /* Bind the request-socket */
    if (bind(listen_sock, (struct sockaddr *)&serveraddr, sizeof serveraddr) < 0) {
        perror("bind");
    exit(1);
    }

    return listen_sock;

}

/* The server program takes a port number as an input parameter, creates a listening socket,
   and then listens for incoming requests. Whenever a new request is received, it spawns a new
   thread to handle the transfer of data, and continues to listen for further requests. The
   amount of data to transfer is decided by drawing a random value from a Pareto distribution.
   This is a so-called heavy-tail distribution, and the distribution is read into an array from
   a sample file, at the beginning of the program. The transfer values are then drawn from this
   array.
*/
int main(int argc, char *argv[]){

  /* Initialize the send buffer with 50000 'q'-characters to transmit */
  int j;
   for (j = 0; j<50000;j++){
        buffer[j] = 'q';
  }

  struct sockaddr_in clientaddr;
  int listen_sock, clientaddrlen = sizeof(struct sockaddr_in);
  int port;
  int err;
  pthread_t test_server;
  long sampleindex = 0;

  if(argc != 4){
    fprintf(stderr,
            "Usage: %s <HTTP Server port> <file with transfer sizes> <type of service> \n", argv[0]);
    exit(1);
  }

  port = atoi(argv[1]);
  mode = atoi(argv[3]);

  initialize_distribution(argv[2]); //read the the Pareto distribution from file

  printf("** START HTTP SERVER **\n");
  listen_sock = create_listen_socket(port); //create a listen socket on the provided port number

  if (listen(listen_sock, SOMAXCONN) < 0) {
    perror("listen");
    exit(1);
  }

  /* Run in a while-loop to handle incoming requests. We need to keep a reference to
     the different sockets as the requests come in, so we keep them in an array. This is to keep
     the threads from stepping on each others "toes", or sockets as we call them in this context..
     Theoretically we can handle 100 000 requests simultaneously, but this never happens in
     practice. Thus, we can reuse the array locations (starting from the front of the array) when
     we reach 100 000, without worrying about ruining the execution of another active thread
  */
  do{
    if(sampleindex == 100000) sampleindex = 0;

    /* Listen for incoming requests */
    if((sockets[sampleindex] = accept(listen_sock,(struct sockaddr *)&clientaddr,
                            &clientaddrlen)) < 0){

      perror("accept");
      exit(1);
    }

    /* Create a new thread to handle the request */
    if((err = pthread_create( &(test_server), NULL,
                              (void*)send_to_socket, (void*)sampleindex)) != 0){

      perror("pthread_create:");
      }

    sampleindex++;

  }while(1);

  close(listen_sock);
}
