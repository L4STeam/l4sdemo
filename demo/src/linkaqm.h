#ifndef LINKQAQM_H
#define LINKQAQM_H

#include "trafficplot.h"
#include "historyplot.h"

#include <QGroupBox>
#include <QLabel>
#include <QComboBox>
#include <vector>

class QLabel;

class Linkaqm : public QGroupBox
{
    Q_OBJECT
public:
    explicit Linkaqm(QWidget *parent = 0);

signals:
    void scheduleRepaint();
    void linkChanged(int);
    int brttChanged(int);


public slots:
    void updateData(std::vector<double> yData_ll, std::vector<double> yData_c, double mark_perc_ll, double drop_perc_ll, double mark_perc_c, double drop_perc_c, int util,
                    double avqqs_ll, double avgqs_c, double p99qs_ll, double p99qs_c);
    void commitData();
    void updateAQM(int num);
    void updateLink(int num);
    void updateBrtt(int num);
    void updateComboLLMark(int num);
    void updateComboLLDrop(int num);
    void updateComboCMark(int num);
    void updateComboCDrop(int num);

private:
    void updatePlotScale(HistoryPlot *plot, int num);
    void readAQMList();
    void readLinkCapList();
    void readBrttList();
    void update_link_properties();

    QMutex dataMutex;

    HistoryPlot *plotUtilHistory;;
    int utilValue;

    TrafficPlot *plotLLQueue;
    TrafficPlot *plotCQueue;

    HistoryPlot *plotMarkHistoryLL;
    HistoryPlot *plotMarkHistoryC;
    HistoryPlot *plotDropHistoryLL;
    HistoryPlot *plotDropHistoryC;

    QComboBox *aqmselect;
    QComboBox *linkselect;
    QComboBox *brttselect;
    QComboBox *combollmark;
    QComboBox *combolldrop;
    QComboBox *combocmark;
    QComboBox *combocdrop;

    QLabel *displayMarkLL;
    QLabel *displayMarkC;
    QLabel *displayDropLL;
    QLabel *displayDropC;
    QLabel *displayUtil;
    QLabel *displayAvgQSLL;
    QLabel *displayAvgQSC;
    QLabel *displayP99QSLL;
    QLabel *displayP99QSC;

    std::vector<double> m_yData_ll;
    std::vector<double> m_yData_c;
    double m_mark_perc_ll;
    double m_drop_perc_ll;
    double m_mark_perc_c;
    double m_drop_perc_c;
    double m_avgqs_ll;
    double m_avgqs_c;
    double m_p99qs_ll;
    double m_p99qs_c;

    QStringList aqmList;
    QStringList brttList;

    QStringList linkCapList;

    std::vector<std::string> aqmIDList;
    std::vector<int> brttValues;
    std::vector<int> linkCapValues;

};

#endif // LINKQAQM_H
