/*
   This program starts a specified number of infinite downloading client threads
   which will connect to a server on a specified port.

   The number of simultanious clients is a command line parameter.

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


void error(char *msg)
{
    perror(msg);
    exit(-1);
}


int port; // port to connect to
char *host; // host to connect to
int number_of_clients;

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
  char buffer[BUFFER_SIZE];

  sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

  /* Make socket reusable */
  int reuse = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

  memset(&addr, 0, sizeof(struct sockaddr_in));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = inet_addr(host);

  /* Connect to host */
  if((err = connect(sock, (struct sockaddr *)&addr, sizeof addr)) < 0){
    error("error connecting to server, thread exiting");  // stops/exits here
  }

  /* Read data for ever or fail */

  while(read_data){
    err = recv(sock, buffer, BUFFER_SIZE, 0);
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
  printf("received bytes: %d", received_bytes);

  close(sock);

  exit(-1);
}

/* This function is responsible for running clients */

int run_dlclients(){

  int err;
  pthread_t thread_client;
  int i = 1;  // start one less in the loop, as the main thread will run one.

  printf("Starting %d client downloads\n", number_of_clients);

  while(i < number_of_clients){

	i++;
    if((err = pthread_create( &(thread_client), NULL,
              (void*)run_client, NULL)) != 0){
      error("pthread_create"); // stops/exits here
    }
	
  }
  // end with blocking on last thread: call one instance in this thread
  run_client();
}

int main(int argc, char *argv[]){

  if(argc != 4){
    fprintf(stderr,
        "Usage: %s <DL Server address> <DL Server port> <number of parallel TCP clients>\n", argv[0]);

    exit(1);
  }

  host = argv[1];
  port = atoi(argv[2]);
  number_of_clients = atoi(argv[3]);

  printf("nr of clients: %d \n", number_of_clients);

  run_dlclients();

}
