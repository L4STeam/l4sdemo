#ifndef TRAFFICANALYZERSTAT_H
#define TRAFFICANALYZERSTAT_H

#include <QObject>
#include "analyzer.h"
#include "demodata.h"

class TrafficAnalyzerStat: public QObject
{
    Q_OBJECT
public:
    TrafficAnalyzerStat(ThreadParam* param, DemoData* demodata);
public slots:
    void start();
signals:
    void dataReady();
private:
    void getQSStat();
    void calcWindow(std::vector<double> *th, std::vector<double> *w, double avg_qs, double rtt);
    void getRateDropMarkStat();

    ThreadParam* tp;
    DemoData* dd;
    int dl_port;
};

#endif // TRAFFICANALYZERSTAT_H
