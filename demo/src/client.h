#ifndef CLIENT_H
#define CLIENT_H

#include <QColor>
#include <QGroupBox>
#include <QMutex>
#include <qwt_plot.h>
#include <qwt_plot_multi_barchart.h>
#include <qwt_plot_barchart.h>
#include <QComboBox>
#include <qwt_plot.h>
#include <qwt_plot_spectrocurve.h>
#include <QVector>
#include <qwt_point_3d.h>
#include <qwt_scale_engine.h>
#include <QCheckBox>
#include <QRadioButton>


class QLabel;
class NoScaleDraw;
class UnrelatedNumberScaleDraw;

class Client : public QGroupBox
{
    Q_OBJECT
public:
    explicit Client(QWidget *parent, const char* download_path,
            const char* killall_path, const char* wb_path,
            const char* rtt_path, const char* cc_path,
            const char* al_path,
            const char* cbr_path,
            const QColor &color = Qt::blue, int init_cc=0);
    ~Client();

signals:
    void scheduleReplot();
    int ccChanged(std::string name);
    int rttChanged(int);

public slots:
    void updateSamples(std::vector<double> rsamples, double cbr_rate,
               double al_rate, QVector<QwtPoint3D>& csamples,
               QVector<QwtPoint3D>& csamples_hs);
    void updateFairRate(double rate, QString caption);

private slots:
    void updateNumDownloads(int num);
    void startDownloads();
    void commitData();
    void updateWebBrowsing0(bool toggled);
    void updateWebBrowsing10(bool toggled);
    void updateWebBrowsing100(bool toggled);
    void updateQos(int num);
    void updateRTT(int num);
    void clearComplData();
    void setComplHS(bool value);
    void setComplWOHS(bool value);
    void updateAL(bool value);
    void updateCBR(int value);
    void updateLinkCap(int value);
    void updateCC(int value);


private:
    void _setCC(const std::string name);
    void readRTTList();
    void readCBRList();
    void readCCList();
    void updateComplHS(bool value);
    int getCBRRate(int value);
    int getLinkCap();
    void cleanup() const;


    int linkcap;

    QwtText lbl;
    QLabel *displayNumDownloads;
    NoScaleDraw *noScaleDraw;
    NoScaleDraw *noScaleDrawAL;
    NoScaleDraw *noScaleDrawCBR;

    UnrelatedNumberScaleDraw  *unrelatedNumberScaleDraw;
    QVector< QVector< double > > samples;
    QVector< double > sampleAL;
    QVector< double > sampleCBR;


    QwtPlot *plotWidget;
    QwtPlot *plotWidgetAL;
    QwtPlot *plotWidgetCBR;
    QwtPlotMultiBarChart *barChart;
    QwtPlotBarChart *ALAppChart;
    QwtPlotBarChart *CBRAppChart;


    QwtPlot *plotCompl;
    QwtPlotSpectroCurve *scatterCompl;
    QwtLogScaleEngine *logscaleX;
    QwtLogScaleEngine *logscaleY;
    QVector<QwtPoint3D> CData;
    QVector<QwtPoint3D> CData_hs;
    std::vector<int> nrComplSamples;
    bool complHS;

    int nrFlows;
    double rate;
    QMutex dataMutex;
    std::string m_serverip;
    std::string m_clientip;
    std::string ssh_killall;
    std::string ssh_download;
    std::string ssh_wb;
    std::string ssh_rtt;
    std::string ssh_al;
    std::string ssh_cbr;
    std::string ssh_cc;

    QString caption;

    QComboBox *qosSelect;
    QComboBox *rttSelect;
    QComboBox *cbrSelect;
    QComboBox *ccSelect;

    QStringList rttList;
    QStringList cbrList;
    QStringList ccList;

    std::vector<int> rttValues;
    std::vector<int> cbrValues;
    std::vector<std::string> ccValues;

    QCheckBox *alCheckb;

    QRadioButton *btnwb0;
    QRadioButton *btnwb10;
    QRadioButton *btnwb100;

};

#endif // CLIENT_H
