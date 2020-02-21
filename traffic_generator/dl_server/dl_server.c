#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
//#include <linux/types.h>
#include <assert.h>
#include <sys/time.h>
// #include <stropts.h> No longer supported, refer to https://bugzilla.redhat.com/show_bug.cgi?id=656245
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

char buffer[1448]; /* Typical MSS on ethernet*/



void send_to_socket(int cur_sock){

    int err;

    while(1){

      if((err = send(cur_sock, buffer, sizeof(buffer), 0)) < 0){

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
        perror("listening socket");
        exit(-1);
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
        exit(-1);
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

  int j;
   for (j = 0; j < sizeof(buffer);j++){
        buffer[j] = 'q';
  }

  struct sockaddr_in clientaddr;
  int listen_sock, clientaddrlen = sizeof(struct sockaddr_in);
  int port;
  int err;
  long socketh;
  pthread_t test_server;

  if(argc != 2){
    fprintf(stderr, "Usage: %s <HTTP Server port>\n", argv[0]);
    exit(-1);
  }

  port = atoi(argv[1]);

  printf("** START DL SERVER **\n");
  listen_sock = create_listen_socket(port); //create a listen socket on the provided port number

  if (listen(listen_sock, SOMAXCONN) < 0) {
    perror("listen");
    exit(1);
  }

  /* Run in a while-loop to handle incoming requests. No need to keep a reference to
     the different sockets as they just have to return rubbish untill the clients stops.
     The server should be able to keep on running, so memory must be cleaned up correctly !
  */
  do{
    /* Listen for incoming requests */
    if((socketh = accept(listen_sock,(struct sockaddr *)&clientaddr,
                            &clientaddrlen)) < 0){

      perror("accept");
      exit(1);
    }

    /* Create a new thread to handle the request */
    if((err = pthread_create( &(test_server), NULL,
                              (void*)send_to_socket, (void*)socketh)) != 0){

      perror("pthread_create:");
      }

  }while(1);

  close(listen_sock); // should not come here
}
