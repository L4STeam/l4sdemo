#ifndef FILLEDCURVE_H
#define FILLEDCURVE_H

#include "qwt_plot_curve.h"

#include <QMutex>
#include <QVector>
#include <QPointF>
#include <QBrush>

class TrafficPlot;

class FilledCurve : public QwtPlotCurve
{
public:
    FilledCurve(TrafficPlot *p, QColor &c);
    void updateData(const std::vector<double>& yData);
    void commitData();
protected:
    virtual void drawCurve (QPainter *p, int style, const QwtScaleMap &xMap, const QwtScaleMap &yMap, const QRectF &canvasRect, int from, int to) const override;
private:
    QMutex vectorMutex;
    TrafficPlot *trafficPlot;
    QVector<QPointF> data;
    std::vector<double> xData;
};

#endif // FILLEDCURVE_H
