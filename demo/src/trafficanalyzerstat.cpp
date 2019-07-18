#include "trafficanalyzerstat.h"
#include "demodata.h"
#include "resources.h"

#include <math.h>

#define percentile(p, n) (round(float(p)/100*float(n)+float(1)/2))

static inline double classic_rtt(const DemoData *dd)
{
	return (double)dd->avg_qsize_c / 1000 + dd->rtt_base + dd->rtt_nonecn;
}

static inline double l4s_rtt(const DemoData *dd)
{
	return (double)dd->avg_qsize_ll / 1000 + dd->rtt_base + dd->rtt_ecn;
}

TrafficAnalyzerStat::TrafficAnalyzerStat(ThreadParam* param,
					 DemoData* demodata)
	: tp(param)
	, dd(demodata)
	, dl_port(std::stoi(safe_getenv("DL_PORT", "5555")))
{
}

void TrafficAnalyzerStat::getQSStat()
{
    std::vector<uint64_t> qs_ecn;
    std::vector<uint64_t> qs_nonecn;
    uint64_t qs_ecn_tot = 0;
    uint64_t qs_nonecn_tot = 0;

    for (int i = 0; i < QS_LIMIT; ++i) {
         if (tp->db2->qs.ecn00[i] > 0 || tp->db2->qs.ecn01[i] > 0 || tp->db2->qs.ecn10[i] > 0 || tp->db2->qs.ecn11[i] > 0) {
             qs_nonecn_tot += tp->db2->qs.ecn00[i]*tp->qdelay_decode_table[i];
             for (uint32_t j = 0; j < tp->db2->qs.ecn00[i]; ++j) {
                 int qdelay = tp->qdelay_decode_table[i];
                 qs_nonecn.push_back(qdelay);
                 int index = (int)round((double)qdelay/1000);
                 if (index < DEMO_QLIM)
                    dd->c_qsize_y.at((int)round((double)qdelay/1000))++;
             }
             qs_ecn_tot += (tp->db2->qs.ecn01[i] + tp->db2->qs.ecn10[i] + tp->db2->qs.ecn11[i])*tp->qdelay_decode_table[i];
             for (uint32_t j = 0; j < (tp->db2->qs.ecn01[i] + tp->db2->qs.ecn10[i] + tp->db2->qs.ecn11[i]); ++j){
                 int qdelay = tp->qdelay_decode_table[i];
                 qs_ecn.push_back(qdelay);
                 int index = (int)round((double)qdelay/1000);
                 if (index < DEMO_QLIM)
                    dd->ll_qsize_y.at((int)round((double)qdelay/1000))++;
             }
         }
    }
    std::sort(qs_ecn.begin(), qs_ecn.end());
    std::sort(qs_nonecn.begin(), qs_nonecn.end());

    uint64_t nrsamples_ecn = qs_ecn.size();
    uint64_t nrsamples_nonecn = qs_nonecn.size();
    uint64_t qs_ecn_avg = 0;
    uint64_t qs_nonecn_avg = 0;
    if (nrsamples_ecn > 0) {
        qs_ecn_avg = qs_ecn_tot / nrsamples_ecn;
        dd->avg_qsize_ll = (double) qs_ecn_avg / 1000;
        dd->p99_qsize_ll = (double) qs_ecn.at(percentile(99,nrsamples_ecn) - 1) / 1000;

    }
    if (nrsamples_nonecn > 0){
        qs_nonecn_avg = qs_nonecn_tot / nrsamples_nonecn;
        dd->avg_qsize_c = (double) qs_nonecn_avg / 1000;
        dd->p99_qsize_c = (double) qs_nonecn.at(percentile(99,nrsamples_nonecn) - 1) / 1000;
    }

    dd->packets_ecn = nrsamples_ecn;
    dd->packets_nonecn = nrsamples_nonecn;

    // normalise to show inverted qs CDF in %
    if (nrsamples_nonecn > 0) {
        double prev = 0;
        for (auto it = dd->c_qsize_y.begin(); it != dd->c_qsize_y.end(); ++it) {
                *it = *it * 100 / nrsamples_nonecn + prev;
                prev = *it;
                *it = 100 - *it;
        }
    }
    if (nrsamples_ecn > 0) {
        double prev = 0;
        for (auto it = dd->ll_qsize_y.begin(); it != dd->ll_qsize_y.end(); ++it) {
                *it = *it * 100 / nrsamples_ecn + prev;
                prev = *it;
                *it = 100 - *it;
	}
    }
}

void TrafficAnalyzerStat::calcWindow(std::vector<double> *th,
				     std::vector<double> *w,
				     double rtt)
{
    int flowid = 0;
    for (auto flow = th->begin(); flow != th->end(); ++flow){
        double window = *flow * rtt * 100 / dd->fair_window;
	if (!tp->quiet)
		printf("fair window %lf\trtt: %lf\n", window, rtt);
        w->at(flowid++) = window;
        *flow = *flow * 100 / dd->fair_rate;
    }
}

void TrafficAnalyzerStat::getRateDropMarkStat()
{
    uint64_t rate_ecn = 0;
    uint64_t rate_nonecn = 0;
    uint64_t drops_ecn = 0;
    uint64_t drops_nonecn = 0;
    uint64_t marks_ecn = 0;
    int tot_greedy_flows = 0;
    int greedy_flows_ecn = 0;
    int greedy_flows_nonecn = 0;
    int al_flows_ecn = 0;
    int al_flows_nonecn = 0;

    for (auto const& val: tp->fd_pf_ecn) {
         rate_ecn += val.second.at(tp->sample_id).rate/8;
         drops_ecn += val.second.at(tp->sample_id).drops;
         marks_ecn += val.second.at(tp->sample_id).marks;

         if (val.first.m_proto == IPPROTO_UDP)
             dd->cbrrate_ecn += (double)val.second.at(tp->sample_id).rate/8;
         else if (val.first.m_srcport == dl_port) {
             double rate = (double)val.second.at(tp->sample_id).rate/8;
             if (greedy_flows_ecn < 10 && rate > 0)
                dd->ecn_th.at(greedy_flows_ecn) = rate;
             if (rate > 0)
                greedy_flows_ecn++;
         } else if (val.first.m_srcport == 11000 ||val.first.m_srcport == 80 || val.first.m_srcport == 9999) {
             dd->alrate_ecn += (double)val.second.at(tp->sample_id).rate/8;
             al_flows_ecn++;
         }

    }
    for (auto const& val: tp->fd_pf_nonecn) {
         rate_nonecn += val.second.at(tp->sample_id).rate/8;
         drops_nonecn += val.second.at(tp->sample_id).drops;
         if (val.first.m_proto == IPPROTO_UDP)
             dd->cbrrate_nonecn += val.second.at(tp->sample_id).rate/8;
         else if (val.first.m_srcport == dl_port) {
             double rate = (double)val.second.at(tp->sample_id).rate/8;
             if (greedy_flows_nonecn < 10 && rate > 0)
                dd->nonecn_th.at(greedy_flows_nonecn) = rate;
             if (rate > 0)
                greedy_flows_nonecn++;
         } else if (val.first.m_srcport == 11000 ||val.first.m_srcport == 80 || val.first.m_srcport == 9999) {
             dd->alrate_nonecn += (double)val.second.at(tp->sample_id).rate/8;
             al_flows_nonecn++;
         }
    }

    double scale = 0.01;
    if (dd->packets_ecn > 0)
        dd->mark_ecn = floor((double)marks_ecn * 100
			     / dd->packets_ecn / scale + 0.5) * scale;
    if (dd->packets_ecn + drops_ecn > 0)
            dd->drop_ecn = floor((double)drops_ecn * 100
				 / (dd->packets_ecn + drops_ecn)
				 / scale + 0.5) * scale;
    if (dd->packets_nonecn + drops_nonecn > 0) {
        dd->drop_nonecn = floor((double)drops_nonecn * 100
				/ (dd->packets_nonecn + drops_nonecn)
				/ scale + 0.5) * scale;
    }
    dd->util = ceil((double)(rate_ecn + rate_nonecn)*100 / dd->linkcap);
    /* if (dd->util > 100) */
	/* dd->util = 100; */

    double remaining_bw = dd->linkcap - dd->cbrrate_ecn - dd->cbrrate_nonecn;
    tot_greedy_flows = greedy_flows_ecn + greedy_flows_nonecn;
    if (tot_greedy_flows > 0) {
        remaining_bw = remaining_bw - dd->alrate_ecn - dd->alrate_nonecn;

        dd->fair_rate = remaining_bw / tot_greedy_flows;
        dd->fair_window = remaining_bw /
		((double)greedy_flows_ecn / l4s_rtt(dd) +
		 (double)greedy_flows_nonecn / classic_rtt(dd));

        calcWindow(&dd->ecn_th, &dd->ecn_w, l4s_rtt(dd));
        calcWindow(&dd->nonecn_th, &dd->nonecn_w, classic_rtt(dd));

     } else {
        dd->fair_rate = remaining_bw;
        if (al_flows_ecn + al_flows_nonecn > 0) {
            dd->fair_rate /= (al_flows_ecn + al_flows_nonecn);
            dd->fair_window = remaining_bw /
		    (al_flows_ecn / l4s_rtt(dd) +
		     al_flows_nonecn / classic_rtt(dd));
        }
    }

    dd->alw_ecn = dd->alrate_ecn * l4s_rtt(dd) * 100 / dd->fair_window;
    dd->alw_nonecn = dd->alrate_nonecn * classic_rtt(dd) * 100 / dd->fair_window;

    dd->alrate_ecn = dd->alrate_ecn * 100 / dd->fair_rate;
    dd->alrate_nonecn = dd->alrate_nonecn * 100 / dd->fair_rate;

    dd->cbrrate_ecn = dd->cbrrate_ecn * 100 / dd->linkcap;
    dd->cbrrate_nonecn = dd->cbrrate_nonecn * 100 / dd->linkcap;

    // Convert to Mbps from bytes per sec (value * 8 / 1000000)
    dd->fair_rate /= 125000;
    // Convert to MTU-sized packets
    dd->fair_window /= 1514.0 * 8;
}

void TrafficAnalyzerStat::start()
{
    uint64_t time_ms, elapsed, next;

    // first run
    // to get accurate results we swap the database and initialize timers here
    // (this way we don't time wrong and gets packets outside our time area)
    tp->db2->init();
    tp->swapDB();
    tp->start = tp->db1->start;

    wait(tp->m_sinterval * NSEC_PER_MS);


    while(1) {
        tp->swapDB();

        // time since we started processing
        time_ms = (tp->db2->last - tp->start) / 1000;
        tp->sample_times.push_back(time_ms);

        pthread_mutex_lock(&dd->mutex);

        dd->init();
        getQSStat();
        processFD();
        getRateDropMarkStat();

        pthread_mutex_unlock(&dd->mutex);
        dataReady();
        tp->db2->init(); // init outside the critical area to save time

        elapsed = getStamp() - tp->start;
        next = ((uint64_t) tp->sample_id + 2) * tp->m_sinterval * 1000; // convert ms to us

        uint64_t process_time = getStamp() - tp->db2->last;
        if (elapsed < next) {
            uint64_t sleeptime = next - elapsed;
	    if (process_time >= sleeptime && !tp->quiet)
		printf("Processed data in approx. %d us - sleeping for %d us\n",
		       (int) process_time, (int) sleeptime);
            wait(sleeptime * NSEC_PER_US);
        }

      /*  pthread_mutex_lock(&tp->quit_lock);
        if (tp->quit) {
            break;
        }
        pthread_mutex_unlock(&tp->quit_lock);*/
        tp->sample_id++;

    }
}


