#ifndef HISTORYPLOT_H
#define HISTORYPLOT_H

#include "curve.h"

#include <qwt_plot.h>

#include <vector>

class DataGenerator;

class HistoryPlot : public QwtPlot
{
    Q_OBJECT
public:
    HistoryPlot(QWidget *parent, QColor &c);
public slots:
    void scheduleReplot();
    void executeReplotForCurve(Curve *curve);
    void updateCurve(double y);
    void updateAxisScale(int xmax, int step);
signals:
    void scheduleReplotForCurve(Curve*);

private:
    Curve *curve1;
    std::vector<double> yData;
    bool autoscale;
    int autox;
};

#endif // HISTORYPLOT_H
