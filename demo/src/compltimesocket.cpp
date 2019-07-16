#include "compltimesocket.h"

#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>

#define BUFFER_SIZE 80000

ComplTimeSocket::ComplTimeSocket(int port): portnr(port), is_open(false)
{
    pthread_mutexattr_t errorcheck;
    pthread_mutexattr_init(&errorcheck);
    pthread_mutex_init(&data_mutex, &errorcheck);
}

ComplTimeSocket::~ComplTimeSocket()
{
     std::cerr << "closing sockets" << std::endl;
     is_open = false;
     close(newsockfd);
}

void ComplTimeSocket::start()
{
    socklen_t clilen;

    struct sockaddr_in serv_addr, cli_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        std::cerr << "ERROR opening socket for port " << portnr;
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portnr);

    int tr=1;
    if (setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&tr,sizeof(int)) == -1){
	std::stringstream ss;
        ss << "ERROR setting sock opt on port " << portnr;
	perror(ss.str().c_str());
        close(sockfd);
        return;
    }

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
	std::stringstream ss;
        ss << "ERROR when binding on port " << portnr;
	perror(ss.str().c_str());
        close(sockfd);
        return;
    }
    listen(sockfd,5);
    clilen = sizeof(cli_addr);

    is_open = true;
accept:
    if (!is_open)
	    return;
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if (newsockfd < 0){
        close(newsockfd);
	std::stringstream ss;
        ss << "ERROR on accept on port " << portnr;
	perror(ss.str().c_str());
        return;
    }
    close(sockfd);

    while (newsockfd >= 0 && is_open) {
        char buffer[BUFFER_SIZE];
        int nr_samples = 0;
        int n;
        int bytes_to_get = 0;
        int bytesrcv = 0;

        n = read(newsockfd,&bytes_to_get,sizeof(int));
        if (n < (int)sizeof(int)) {
            close(newsockfd);
            goto accept;
        }
        n = 0;
        while (bytesrcv < bytes_to_get) {
            n = read(newsockfd,&buffer[bytesrcv],bytes_to_get-bytesrcv);
            if (n < 0) {
                close(newsockfd);
                goto accept;
            }
            bytesrcv += n;
            n = 0;
        }
        if (n < 0) {
            close(newsockfd);
            goto accept;
        }
        nr_samples = bytesrcv/3/sizeof(double);
        double* dataptr = (double*)buffer;
        pthread_mutex_lock(&data_mutex);
        for (int i = 0; i < nr_samples; ++i) {
            QwtPoint3D p(dataptr[i],dataptr[i+nr_samples]/1000000.0f, 1000);
            QwtPoint3D p_hs(dataptr[i],dataptr[i+nr_samples*2]/1000000.0f, 1000);
            compl_data.append(p);
            compl_data_hs.append(p_hs);

        }
        pthread_mutex_unlock(&data_mutex);
     }
}

void ComplTimeSocket::getData(QVector<QwtPoint3D> *data, QVector<QwtPoint3D> *data_hs)
{
    pthread_mutex_lock(&data_mutex);
    *data = compl_data;
    *data_hs = compl_data_hs;
    compl_data.clear();
    compl_data_hs.clear();
    pthread_mutex_unlock(&data_mutex);
}

