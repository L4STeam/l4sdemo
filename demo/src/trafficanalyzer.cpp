#include "trafficanalyzer.h"
#include "analyzer.h"

#include <iostream>
#include <QThread>


TrafficAnalyzer::TrafficAnalyzer(QObject *parent, ThreadParam *param, DemoData *demodata, bool cl)
    : QObject(parent)
    , demo_data(demodata)
    , ipc (cl)
    , tp(param)

{
}

void TrafficAnalyzer::start()
{

    pthread_t thread_id = 0;
    pthread_attr_t attrs;
    pthread_attr_init(&attrs);
    pthread_attr_setdetachstate(&attrs, PTHREAD_CREATE_JOINABLE);

    pcapLoop(NULL);
    std::cout << "exiting ta thread" << std::endl;
    QThread *mythread = this->thread();
	mythread->exit();
}



