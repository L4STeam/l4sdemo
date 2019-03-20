#include "curve.h"

#include "historyplot.h"

#include <QBrush>
#include <QMutexLocker>
#include <stdio.h>


Curve::Curve(HistoryPlot *p, QColor &c)
    : historyPlot(p)
{
    attach(historyPlot);
    setPen(Qt::red, 1.0 /*line width*/);
    setVisible(true);
    QBrush brush(c, Qt::SolidPattern);
}

void Curve::updateData(const std::vector<double> &yData)
{
    int len = yData.size();
    data.resize(len);
    for (int i = 0; i < len; ++i) {
        data[i] = QPointF(i, yData[i]);
    }
    historyPlot->scheduleReplotForCurve(this);
}

void Curve::commitData()
{
      setSamples(data);
}

void Curve::drawCurve(QPainter *p, int style, const QwtScaleMap &xMap, const QwtScaleMap &yMap, const QRectF &canvasRect, int from, int to) const
{
    QwtPlotCurve::drawCurve(p, style, xMap, yMap, canvasRect, from, to);
}
