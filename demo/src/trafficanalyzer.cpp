#include "trafficanalyzer.h"
#include "analyzer.h"

#include <iostream>
#include <QThread>


TrafficAnalyzer::TrafficAnalyzer(QObject *parent, ThreadParam *param,
				 DemoData *demodata)
    : QObject(parent)
    , tp(param)
    , demo_data(demodata){}

void TrafficAnalyzer::start()
{

    pthread_attr_t attrs;
    pthread_attr_init(&attrs);
    pthread_attr_setdetachstate(&attrs, PTHREAD_CREATE_JOINABLE);

    pcapLoop(NULL);
    std::cout << "exiting ta thread" << std::endl;
    this->thread()->exit();
}