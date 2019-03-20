#ifndef TRAFFICANALYZER_H
#define TRAFFICANALYZER_H


#include "analyzer.h"
#include "demodata.h"

#include <QObject>
#include <QThread>
#include <vector>

class TrafficAnalyzer : public QObject
{
    Q_OBJECT
public:
    explicit TrafficAnalyzer(QObject *parent = 0, ThreadParam *param = 0, DemoData *demodata = 0, bool cl = 0);

public slots:
    void start();

private:
    ThreadParam *tp;
    DemoData *demo_data;
    bool ipc;
};

#endif // TRAFFICANALYZER_H
