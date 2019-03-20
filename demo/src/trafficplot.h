#ifndef TRAFFICPLOT_H
#define TRAFFICPLOT_H

#include "filledcurve.h"

#include <qwt_plot.h>

#include <vector>

class DataGenerator;

class TrafficPlot : public QwtPlot
{
    Q_OBJECT
public:
    TrafficPlot(QWidget *parent, QColor &c);
public slots:
    void scheduleReplot();
    void executeReplotForCurve(FilledCurve *curve);
    void updateCurve(std::vector<double> yData);
signals:
    void scheduleReplotForCurve(FilledCurve*);

private:
    FilledCurve *curve1;
};

#endif // TRAFFICPLOT_H
