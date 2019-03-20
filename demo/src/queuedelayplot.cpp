#include "trafficplot.h"

#include "curve.h"
#include "datagenerator.h"

#include <iostream>
#include <QTimer>
#include <QWindow>

DataGenerator *g = 0;
QThread *generatorThread = 0;

QueueDelayPlot::QueueDelayPlot(QWidget *parent)
    : QwtPlot(parent)
    , curve1(0)
{
    setAutoReplot(false);
    curve1 = new Curve(this);
    connect(this, SIGNAL(scheduleReplotForCurve(Curve*)), this, SLOT(executeReplotForCurve(Curve*)));
}

void QueueDelayPlot::scheduleReplot()
{
    QTimer::singleShot(0, this, SLOT(replot()));
}

void QueueDelayPlot::executeReplotForCurve(Curve *curve)
{
    curve->commitData();
    replot();
}



// --------- THIS IS AN EXAMPLE ONLY AND SHOULD BE REMOVED --------------
void QueueDelayPlot::generateTraffic()
{
    // This is leaking... but you will not need the DataGenerator anyway.
    // So just delete the crap!
    generatorThread = new QThread;
    g = new DataGenerator(0, curve1);
    g->moveToThread(generatorThread);
    connect(generatorThread, SIGNAL(started()), g, SLOT(start()));
    generatorThread->start();
}
