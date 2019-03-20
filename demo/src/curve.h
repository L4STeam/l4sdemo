#ifndef CURVE_H
#define CURVE_H

#include "qwt_plot_curve.h"

#include <QMutex>
#include <QVector>
#include <QPointF>
#include <QBrush>

class HistoryPlot;

class Curve : public QwtPlotCurve
{
public:
    Curve(HistoryPlot *p, QColor &c);
    void updateData(const std::vector<double>& yData);
    void commitData();
protected:
    virtual void drawCurve (QPainter *p, int style, const QwtScaleMap &xMap, const QwtScaleMap &yMap, const QRectF &canvasRect, int from, int to) const override;
private:
    QMutex vectorMutex;
    HistoryPlot *historyPlot;
    QVector<QPointF> data;
    std::vector<double> xData;
};

#endif // CURVE_H
