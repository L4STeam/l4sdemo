#include "trafficplot.h"

#include "filledcurve.h"
#include "datagenerator.h"

#include <iostream>
#include <QTimer>
#include <QWindow>
#include <vector>

DataGenerator *g = 0;
QThread *generatorThread = 0;

TrafficPlot::TrafficPlot(QWidget *parent, QColor &c)
    : QwtPlot(parent)
    , curve1(0)
{
    QFont axisFont("Times New Roman", 10);
    QFont titleFont("Helvetica", 12);
    titleFont.setBold(true);

    this->setAxisScale(0, 0, 100, 20);
    this->setAxisScale(2, 0, 140, 20);
    QwtText title;
    title.setText("Queue filling [%]");
    title.setFont(titleFont);
    this->setAxisTitle(0, title);
    this->setAxisFont(0, axisFont);
    this->setAxisFont(2, axisFont);

    this->setMaximumHeight(200);
    this->setMinimumWidth(260);
  
    setAutoReplot(false);
    curve1 = new FilledCurve(this, c);
    curve1->setPen(c, 1.0 /*line width*/);
    connect(this, SIGNAL(scheduleReplotForCurve(FilledCurve*)), this, SLOT(executeReplotForCurve(FilledCurve*)));
}

void TrafficPlot::scheduleReplot()
{
    QTimer::singleShot(0, this, SLOT(replot()));
}

void TrafficPlot::executeReplotForCurve(FilledCurve *curve)
{
    curve->commitData();
    replot();
}

void TrafficPlot::updateCurve(std::vector<double> yData)
{
    curve1->updateData(yData);
    executeReplotForCurve(curve1);
}

