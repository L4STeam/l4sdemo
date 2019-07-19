#include "datagenerator.h"
#include "resources.h"

#include <iostream>
#include <cstdlib>
#include <unistd.h>

DataGenerator::DataGenerator(Client *dctcpclient, Client *cubicclient,
			     Linkaqm *linkaqm, std::string iface,
			     std::string filter)
    : taThread(0)
    , m_dctcpclient(dctcpclient)
    , m_cubicclient(cubicclient)
    , m_linkaqm(linkaqm)
    , pcap_iface(iface)
    , pcap_filter(filter)
{
     compl_ll_data = new QVector<QwtPoint3D>;
     compl_c_data = new QVector<QwtPoint3D>;
     show_rate = true;
     pthread_mutexattr_t errorcheck;
     pthread_mutexattr_init(&errorcheck);
     pthread_mutex_init(&showrate_mutex, &errorcheck);
     connect(dctcpclient, SIGNAL(rttChanged(int)), this, SLOT(updateDctcpRtt(int)));
     connect(cubicclient, SIGNAL(rttChanged(int)), this, SLOT(updateCubicRtt(int)));
     connect(linkaqm, SIGNAL(brttChanged(int)), this, SLOT(updateBaseRtt(int)));

}

DataGenerator::~DataGenerator()
{
     delete compl_ll_data;
     delete compl_c_data;
}

void DataGenerator::startTA(bool ipc)
{
    uint32_t sinterval = 1000;
    uint32_t nrs = 0;
    std::string folder = safe_getenv("TA_DEMO_FOLDER", "demo");

    tp = new ThreadParam(sinterval, folder, false, nrs);
    tp->quiet = !getenv_has_key("VERBOSE");
    setThreadParam(tp);
    demo_data = new DemoData();
    demo_data->ipclass = ipc;
    taThread = new QThread();
    tastatThread = new QThread();
    ta = new TrafficAnalyzer(0, tp, demo_data, ipc);
    tastat = new TrafficAnalyzerStat(tp, demo_data);
    connect(tastat, SIGNAL(dataReady()), this, SLOT(updateData()));
    ta->moveToThread(taThread);
    connect(taThread, SIGNAL(started()), ta, SLOT(start()));
    tastat->moveToThread(tastatThread);
    connect(tastatThread, SIGNAL(started()), tastat, SLOT(start()));

    setup_pcap(tp, pcap_iface.c_str(), pcap_filter);
    taThread->start();
    tastatThread->start();
}

void DataGenerator::updateTA(bool ipc)
{
	pthread_mutex_lock(&tp->m_mutex);
        tp->ipclass = ipc;
        pthread_mutex_unlock(&tp->m_mutex);
}
void DataGenerator::startCompl()
{
    complLLThread = new QThread();
    m_compl_ll = new ComplTimeSocket(safe_getenv("PORT_A", "11000"));
    m_compl_ll->moveToThread(complLLThread);
    connect(complLLThread, SIGNAL(started()), m_compl_ll, SLOT(start()));
    complLLThread->start();

    complCThread = new QThread();
    m_compl_c = new ComplTimeSocket(safe_getenv("PORT_B", "11001"));
    m_compl_c->moveToThread(complCThread);
    connect(complCThread, SIGNAL(started()), m_compl_c, SLOT(start()));
    complCThread->start();
}


void DataGenerator::updateData()
{
       pthread_mutex_lock(&demo_data->mutex);
       DemoData dd = *demo_data;
       pthread_mutex_unlock(&demo_data->mutex);
       // update data
       QVector<QwtPoint3D> dataC;
       QVector<QwtPoint3D> dataC_hs;
       QVector<QwtPoint3D> dataLL;
       QVector<QwtPoint3D> dataLL_hs;
       m_compl_c->getData(&dataC, &dataC_hs);
       m_compl_ll->getData(&dataLL, &dataLL_hs);

       pthread_mutex_lock(&showrate_mutex);
       if (show_rate) {
           m_dctcpclient->updateFairRate(dd.fair_rate, QStringLiteral(" Mbps"));
           m_cubicclient->updateFairRate(dd.fair_rate, QStringLiteral(" Mbps"));
           m_dctcpclient->updateSamples(dd.ecn_th, dd.cbrrate_ecn, dd.alrate_ecn, dataLL, dataLL_hs);
           m_cubicclient->updateSamples(dd.nonecn_th, dd.cbrrate_nonecn, dd.alrate_nonecn, dataC, dataC_hs);
       } else {
           m_dctcpclient->updateFairRate(dd.fair_window, QStringLiteral(" Packets"));
           m_cubicclient->updateFairRate(dd.fair_window, QStringLiteral(" Packets"));
           m_dctcpclient->updateSamples(dd.ecn_w, dd.cbrrate_ecn, dd.alw_ecn, dataLL, dataLL_hs);
           m_cubicclient->updateSamples(dd.nonecn_w, dd.cbrrate_nonecn, dd.alw_nonecn, dataC, dataC_hs);
       }

       pthread_mutex_unlock(&showrate_mutex);
       m_linkaqm->updateData(dd.ll_qsize_y, dd.c_qsize_y, dd.mark_ecn, dd.drop_ecn, dd.mark_nonecn, dd.drop_nonecn, (int)dd.util,
                             dd.avg_qsize_ll, dd.avg_qsize_c, dd.p99_qsize_ll, dd.p99_qsize_c);
   }

void DataGenerator::setShowRate(bool toggled)
{
    std::cerr << "R toggled " << toggled << std::endl;

    pthread_mutex_lock(&showrate_mutex);
    show_rate = toggled;
    pthread_mutex_unlock(&showrate_mutex);
}

void DataGenerator::setShowWindow(bool toggled)
{
    std::cerr << "W toggled " << toggled << std::endl;
    pthread_mutex_lock(&showrate_mutex);
    if (toggled == true)
        show_rate = false;
    else
        show_rate = true;

    pthread_mutex_unlock(&showrate_mutex);
}
void DataGenerator::setECNClass(bool toggled)
{
    if (toggled == true)
        updateTA(false);
}
void DataGenerator::setIPClass(bool toggled)
{
    if (toggled == true)
        updateTA(true);
}
void DataGenerator::updateLinkCap(int linkcap)
{
     pthread_mutex_lock(&demo_data->mutex);
     demo_data->linkcap = (double)linkcap * 1000000.0;
     pthread_mutex_unlock(&demo_data->mutex);
}

void DataGenerator::updateDctcpRtt(int value)
{
    pthread_mutex_lock(&demo_data->mutex);
    demo_data->rtt_ecn = (double)value / 1000.0;
    pthread_mutex_unlock(&demo_data->mutex);
}

void DataGenerator::updateCubicRtt(int value)
{
    pthread_mutex_lock(&demo_data->mutex);
    demo_data->rtt_nonecn = (double)value / 1000.0;
    pthread_mutex_unlock(&demo_data->mutex);
}

void DataGenerator::updateBaseRtt(int value)
{
    pthread_mutex_lock(&demo_data->mutex);
    demo_data->rtt_base = (double)value / 1000.0;
    pthread_mutex_unlock(&demo_data->mutex);
}
