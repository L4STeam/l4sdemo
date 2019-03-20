#include "filledcurve.h"

#include "trafficplot.h"

#include <QBrush>
#include <QMutexLocker>
#include <stdio.h>

FilledCurve::FilledCurve(TrafficPlot *p, QColor &c)
    : trafficPlot(p)
{
    attach(trafficPlot);
    setVisible(true);
    QBrush brush(c, Qt::SolidPattern);
    this->setBrush(brush);
    for (double i = 0; i < 2048; ++i)
            xData.push_back(i);
}

void FilledCurve::updateData(const std::vector<double> &yData)
{
    int len = yData.size();
    data.resize(len);
    for (int i = 0; i < len; ++i) {
        data[i] = QPointF(xData[i], yData[i]);
    }
    trafficPlot->scheduleReplotForCurve(this);
}

void FilledCurve::commitData()
{
      setSamples(data);
}

void FilledCurve::drawCurve(QPainter *p, int style, const QwtScaleMap &xMap, const QwtScaleMap &yMap, const QRectF &canvasRect, int from, int to) const
{
    QwtPlotCurve::drawCurve(p, style, xMap, yMap, canvasRect, from, to);
}
