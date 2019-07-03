#ifndef DATAGENERATOR_H
#define DATAGENERATOR_H

#include "client.h"
#include "linkaqm.h"
#include "demodata.h"
#include "compltimesocket.h"
#include "analyzer.h"
#include "trafficanalyzer.h"
#include "trafficanalyzerstat.h"

#include <QObject>
#include <QThread>
#include <QTimer>
#include <vector>

class Curve;

class DataGenerator : public QObject
{
    Q_OBJECT
public:
    explicit DataGenerator(Client *dctcpclient, Client *cubicclient,
			   Linkaqm *linkqaqm, std::string iface,
			   std::string filter);
    ~DataGenerator();

public slots:
    void startTA(bool ipc = false);
    void updateTA(bool ipc);
    void startCompl();
    void setShowRate(bool toggled);
    void setShowWindow(bool toggled);
    void setECNClass(bool toggled);
    void setIPClass(bool toggled);
    void updateData();
    void updateLinkCap(int linkcap);
    void updateDctcpRtt(int value);
    void updateCubicRtt(int value);
    void updateBaseRtt(int value);

private:
    ThreadParam *tp;
    QThread *taThread;
    QThread *tastatThread;
    TrafficAnalyzer *ta;
    TrafficAnalyzerStat *tastat;
    QThread *complLLThread;
    QThread *complCThread;
    Client *m_dctcpclient;
    Client *m_cubicclient;
    Linkaqm *m_linkaqm;
    ComplTimeSocket *m_compl_ll;
    ComplTimeSocket *m_compl_c;
  //  QTimer *m_timer;
    DemoData *demo_data;
    QVector<QwtPoint3D> *compl_ll_data;
    QVector<QwtPoint3D> *compl_c_data;

    bool show_rate;
    pthread_mutex_t showrate_mutex;
    pthread_mutex_t linkcap_mutex;
    std::string pcap_iface, pcap_filter;
};

#endif // DATAGENERATOR_H
