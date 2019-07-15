#ifndef COMPLTIMESOCKET_H
#define COMPLTIMESOCKET_H


#include "demodata.h"


#include <QObject>
#include <QThread>
#include <QVector>
#include <qwt_point_3d.h>
#include <vector>


class ComplTimeSocket : public QObject
{
    Q_OBJECT
public:
    explicit ComplTimeSocket(int port);
    ~ComplTimeSocket();

public slots:
    void start();
    void getData(QVector<QwtPoint3D> *data, QVector<QwtPoint3D> *data_hs);


private:
    int portnr;
    int sockfd;
    int newsockfd;
    QVector<QwtPoint3D> compl_data;
    QVector<QwtPoint3D> compl_data_hs;
    pthread_mutex_t data_mutex;
    bool is_open;
};

#endif // COMPLTIMESOCKET_H
