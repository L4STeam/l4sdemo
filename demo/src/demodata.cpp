#include "demodata.h"

DemoData::DemoData()
{
    ipclass = 0;
    pthread_mutexattr_t errorcheck;
    pthread_mutexattr_init(&errorcheck);
    pthread_mutex_init(&mutex, &errorcheck);
    pthread_cond_init(&newdata, NULL);
    init();
    linkcap = 5000000;
    rtt_ecn = 0;
    rtt_nonecn = 0;
    rtt_base = 0.007;
}

void DemoData::init()
{
    util = 0;
    mark_ecn = 0;
    drop_ecn = 0;
    mark_nonecn = 0;
    drop_nonecn = 0;
    fair_rate = 0;
    fair_window = 0;
    alrate_ecn = 0;
    alrate_nonecn = 0;
    alw_ecn = 0;
    alw_nonecn = 0;
    cbrrate_ecn = 0;
    cbrrate_nonecn = 0;

    packets_ecn = 0;
    packets_nonecn = 0;

    ecn_th.clear();
    nonecn_th.clear();
    ecn_w.clear();
    nonecn_w.clear();

    ecn_th.resize(10);
    nonecn_th.resize(10);
    ecn_w.resize(10);
    nonecn_w.resize(10);
    avg_qsize_ll = 0;
    avg_qsize_c = 0;
    p99_qsize_ll = 0;
    p99_qsize_c = 0;
    ll_qsize_y.clear();
    c_qsize_y.clear();
    ll_qsize_y.resize(DEMO_QLIM);
    c_qsize_y.resize(DEMO_QLIM);

}
