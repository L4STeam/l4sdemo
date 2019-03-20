#ifndef QUEUEDELAYPLOT_H
#define QUEUEDELAYPLOT_H

#include "curve.h"

#include <qwt_plot.h>

#include <vector>

class DataGenerator;

class QueueDelayPlot : public QwtPlot
{
    Q_OBJECT
public:
    QueueDelayPlot(QWidget *parent = 0);
public slots:
    void scheduleReplot();
    void executeReplotForCurve(Curve *curve);
    void generateTraffic();
signals:
    void scheduleReplotForCurve(Curve*);

private:
    Curve *curve1;
    DataGenerator *dataGenerator;
};

#endif // QUEUEDELAYPLOT_H
