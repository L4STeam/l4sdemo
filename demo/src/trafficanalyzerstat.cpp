#include "trafficanalyzerstat.h"
#include "demodata.h"
#include "resources.h"

#include <math.h>
#include <net/ethernet.h>

static inline int percentile(float p, float n)
{
    return round(p / 100.0 * n + 1.0 / 2);
}

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
	, dl_port(std::stoi(safe_getenv("DL_PORT", "5555"))) {}

void TrafficAnalyzerStat::getQSStat()
{
    std::vector<uint64_t> qs_ecn;
    std::vector<uint64_t> qs_nonecn;
    uint64_t qs_ecn_tot = 0;
    uint64_t qs_nonecn_tot = 0;

    qs_ecn.reserve(tp->db2->tot_packets_ecn);
    qs_nonecn.reserve(tp->db2->tot_packets_nonecn);
    /* Iterate over all possible queue delay values */
    for (int i = 0; i < QS_LIMIT; ++i) {
	const int qdelay = tp->qdelay_decode_table[i];
	/* Increment the buckets by their number of received packets */
        qs_nonecn_tot += tp->db2->qs.ecn00[i] * qdelay;
        for (uint32_t j = 0; j < tp->db2->qs.ecn00[i]; ++j) {
            qs_nonecn.push_back(qdelay);
            const int index = (int)round((double)qdelay / 1000);
            if (index < DEMO_QLIM)
               dd->c_qsize_y.at(index)++;
        }

	const uint32_t ecn_entries = tp->db2->qs.ecn01[i] +
		tp->db2->qs.ecn10[i] + tp->db2->qs.ecn11[i];
        qs_ecn_tot += ecn_entries * qdelay;
        for (uint32_t j = 0; j < ecn_entries; ++j){
            qs_ecn.push_back(qdelay);
            const int index = (int)round((double)qdelay/1000);
            if (index < DEMO_QLIM)
               dd->ll_qsize_y.at(index)++;
        }
    }
    std::sort(qs_ecn.begin(), qs_ecn.end());
    std::sort(qs_nonecn.begin(), qs_nonecn.end());

    if ((dd->packets_ecn = tp->db2->tot_packets_ecn)) {
        dd->avg_qsize_ll = (double)qs_ecn_tot / dd->packets_ecn / 1000.0;
        dd->p99_qsize_ll = (double)qs_ecn.at(
			percentile(99, dd->packets_ecn) - 1) / 1000.0;
	/* Compute CCDF values */
        double prev = 0.0;
        for (auto &it : dd->ll_qsize_y) {
                prev = it * 100.0 / dd->packets_ecn + prev;
                it = 100.0 - prev;
        }
    }
    if ((dd->packets_nonecn = tp->db2->tot_packets_nonecn)) {
        dd->avg_qsize_c = (double)qs_nonecn_tot / dd->packets_nonecn
		/ 1000.0;
        dd->p99_qsize_c = (double)qs_nonecn.at(
			percentile(99, dd->packets_nonecn) - 1) / 1000.0;
	/* Compute CCDF values */
        double prev = 0.0;
        for (auto &it : dd->c_qsize_y) {
                prev = it * 100.0 / dd->packets_nonecn + prev;
                it = 100 - prev;
	}
    }
}

void TrafficAnalyzerStat::calcWindow(std::vector<double> &th,
				     std::vector<double> &w,
				     double rtt)
{
    int flowid = 0;
    for (auto &flow : th) {
        double window = flow * rtt * 100 / dd->fair_window;
	if (!tp->quiet)
		printf("fair window %lf\trtt: %lf\n", window, rtt);
        w.at(flowid++) = window;
        flow = flow * 100 / dd->fair_rate;
    }
}

static inline void _update_drop_mark_stats(int dl_port, int sample_id,
		std::map<SrcDst,std::vector<FlowData>> &fd_pf,
		double *rate, uint64_t *drops, uint64_t *marks, double *cbrrate,
		std::vector<double> &greedy_rate,
		int *greedy_count, int *al_count, double *alrate)
{
    for (auto const& val: fd_pf) {
         double bitrate = val.second.at(sample_id).rate;
         *rate += bitrate;
         *drops += val.second.at(sample_id).drops;
	 if (marks)
		*marks += val.second.at(sample_id).marks;

         if (val.first.m_proto == IPPROTO_UDP)
             *cbrrate += bitrate;
         else if (val.first.m_srcport == dl_port) {
             if (*greedy_count < 10 && bitrate > 0.0)
                greedy_rate.at(*greedy_count) = bitrate;
             if (bitrate > 0.0)
                *greedy_count += 1;
         } else {
		 /*if (val.first.m_srcport == 11000 ||val.first.m_srcport == 80
		  * || val.first.m_srcport == 9999) { */
             *alrate += bitrate;
             *al_count += 1;
         }

    }
}

double _unbiased_percentage(double val, double base)
{
    return base > 0.0 ? floor(val * 100.0 / base / 0.01 + 0.5) * 0.01 : 0.0;
}

void TrafficAnalyzerStat::getRateDropMarkStat()
{
    double rate_ecn = 0;
    double rate_nonecn = 0;
    uint64_t drops_ecn = 0;
    uint64_t drops_nonecn = 0;
    uint64_t marks_ecn = 0;
    int tot_greedy_flows = 0;
    int greedy_flows_ecn = 0;
    int greedy_flows_nonecn = 0;
    int al_flows_ecn = 0;
    int al_flows_nonecn = 0;

    _update_drop_mark_stats(dl_port, tp->sample_id, tp->fd_pf_ecn, &rate_ecn,
			    &drops_ecn, &marks_ecn, &dd->cbrrate_ecn,
			    dd->ecn_th, &greedy_flows_ecn, &al_flows_ecn,
			    &dd->alrate_ecn);
    _update_drop_mark_stats(dl_port, tp->sample_id, tp->fd_pf_nonecn,
			    &rate_nonecn, &drops_nonecn, NULL,
			    &dd->cbrrate_nonecn, dd->nonecn_th,
			    &greedy_flows_nonecn, &al_flows_nonecn,
			    &dd->alrate_nonecn);

    dd->mark_ecn = _unbiased_percentage(marks_ecn, dd->packets_ecn);
    dd->drop_ecn = _unbiased_percentage(drops_ecn,
					dd->packets_ecn + dd->drop_ecn);
    dd->drop_nonecn = _unbiased_percentage(drops_nonecn,
					   dd->packets_nonecn + dd->drop_nonecn);
    dd->util = ceil((double)(rate_ecn + rate_nonecn) * 100.0 / dd->linkcap);
    /* if (dd->util > 100) */
	/* dd->util = 100; */

    double remaining_bw = dd->linkcap - dd->cbrrate_ecn - dd->cbrrate_nonecn;
    if ((tot_greedy_flows = greedy_flows_ecn + greedy_flows_nonecn) > 0) {
        remaining_bw = remaining_bw - dd->alrate_ecn - dd->alrate_nonecn;

        dd->fair_rate = remaining_bw / tot_greedy_flows;
        dd->fair_window = remaining_bw /
		((double)greedy_flows_ecn / l4s_rtt(dd) +
		 (double)greedy_flows_nonecn / classic_rtt(dd));

        calcWindow(dd->ecn_th, dd->ecn_w, l4s_rtt(dd));
        calcWindow(dd->nonecn_th, dd->nonecn_w, classic_rtt(dd));
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

    // Convert to Mbps
    dd->fair_rate /= 1000000.0;
    // Convert to MTU-sized packets, i.e., ethernet MTU bytes
    dd->fair_window /= (ETH_FRAME_LEN << 3);
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


