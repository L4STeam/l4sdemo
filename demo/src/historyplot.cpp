#include "historyplot.h"

#include "filledcurve.h"
#include "datagenerator.h"

#include <iostream>
#include <QTimer>
#include <QWindow>
#include <vector>

HistoryPlot::HistoryPlot(QWidget *parent, QColor &c)
    : QwtPlot(parent)
    , curve1(0)
    , autoscale(1)
    , autox(100)
{
    QFont axisFont("Times New Roman", 10);
    QFont titleFont("Helvetica", 11);
    titleFont.setBold(true);

    this->setMinimumWidth(200);
    axisFont.setBold(false);
    this->setAxisScale(0, 0, 100, 50);
    this->setAxisScale(2, 60, 0, 20);
    this->setAxisFont(0, axisFont);
    this->setAxisFont(2, axisFont);
    QwtText title;
    title.setText("Time [sec]");
    title.setFont(titleFont);
    this->setAxisTitle(2, title);

    this->setMaximumHeight(100);
    this->setMinimumWidth(100);
  
    setAutoReplot(false);
    curve1 = new Curve(this, c);
    curve1->setPen(c, 1.0 /*line width*/);
    connect(this, SIGNAL(scheduleReplotForCurve(Curve*)), this, SLOT(executeReplotForCurve(Curve*)));
}

void HistoryPlot::scheduleReplot()
{
    QTimer::singleShot(0, this, SLOT(replot()));
}

void HistoryPlot::executeReplotForCurve(Curve *curve)
{
    curve->commitData();
    replot();
}

void HistoryPlot::updateCurve(double newy)
{
    std::vector<double>::iterator it = yData.begin();
    yData.insert(it, newy);
    if (yData.size() > 60)
        yData.resize(60);

    if (autoscale) {
        double maxvalue = 0;
        for (it = yData.begin(); it != yData.end(); ++it) {
            if (*it > maxvalue)
                maxvalue = *it;
        }
        if (maxvalue < 10)
            autox = 10;
        else if (maxvalue < 20)
            autox = 20;
        else if (maxvalue < 40)
            autox = 40;
        else if (maxvalue < 60)
            autox = 60;
        else if (maxvalue < 80)
            autox = 80;
        else
            autox = 100;
        this->setAxisScale(0, 0, autox, autox/2);
    }

    curve1->updateData(yData);
    executeReplotForCurve(curve1);
}

void HistoryPlot::updateAxisScale(int xmax, int step)
{
    if (xmax == 0)
        autoscale = 1;
    else {
        autoscale = 0;
        this->setAxisScale(0, 0, xmax, step);
    }
    replot();
}

