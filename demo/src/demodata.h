#ifndef DEMODATA_H
#define DEMODATA_H

#include <QObject>

#define DEMO_QLIM 140

class DemoData
{
public:
    explicit DemoData();

    bool ipclass;
    std::vector<double> ecn_th;
    std::vector<double> nonecn_th;
    std::vector<double> ecn_w;
    std::vector<double> nonecn_w;

    long long util;
    double rtt_base;
    double rtt_ecn;
    double rtt_nonecn;
    double linkcap;
    double mark_ecn;
    double drop_ecn;
    double mark_nonecn;
    double drop_nonecn;
    double fair_rate;
    double fair_window;
    double alrate_ecn;
    double alrate_nonecn;
    double alw_ecn;
    double alw_nonecn;
    double cbrrate_ecn;
    double cbrrate_nonecn;

    uint64_t packets_ecn;
    uint64_t packets_nonecn;

    std::vector<double> ll_qsize_y;
    std::vector<double> c_qsize_y;
    double avg_qsize_ll;
    double avg_qsize_c;
    double p99_qsize_ll;
    double p99_qsize_c;
    pthread_mutex_t mutex;
    pthread_cond_t newdata;
    void init();

};

#endif // DEMODATA_H
