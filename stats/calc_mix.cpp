#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <fstream>
#include <vector>
#include <math.h>
#include <sstream>
#include <string>
#include <map>
#include <unistd.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <string.h>
#include <algorithm>

#define NRSAMPLES 250
#define PDF_BINS 50
#define MAX_QS 2048

#define percentile(p, n) (ceil(float(p)/100*float(n)))

struct Parameters {
    double rtt_a;
    double rtt_b;
    std::string folder;
    uint32_t n_a;
    uint32_t n_b;
    double link;

    Parameters() {
        rtt_a = 0;
        rtt_b = 0;
        folder = "";
        n_a = 0;
        n_b = 0;
        link = 0;
    }
};

class Statistics {
  public:
    Statistics(std::string fn) {
        calculated_variance = false;
        calculated_coeffVar = false;
        _variance = NAN;
        _average = NAN;
        _coeffVar = NAN;
        _samples = NULL;
        filename_out = fn;
    }

    void samples(std::vector<double> *new_samples) {
        _samples = new_samples;
        std::sort(_samples->begin(), _samples->end());
    }

    std::vector<double> *samples() {
        return _samples;
    }

    double p(double p) {
        if (_samples != NULL && _samples->size() > 0) {
            return _samples->at(percentile(p, _samples->size()) - 1);
        }

        return NAN;
    }

    double variance() {
        if (_samples != NULL && !calculated_variance) {
            calculate_variance();
        }

        return _variance;
    }

    double average() {
        if (_samples != NULL && !calculated_variance) {
            calculate_variance();
        }

        return _average;
    }

    double coeffVar() {
        if (_samples != NULL && !calculated_coeffVar) {
            calculate_coeffVar();
        }

        return _coeffVar;
    }

    double stddev() {
        return sqrt(variance());
    }
    std::string filename_out;


  private:
    bool calculated_variance;
    bool calculated_coeffVar;
    long double _variance;
    long double _average;
    long double _coeffVar;
    std::vector<double> *_samples;

    void calculate_coeffVar() {
        if (variance() > 0 && average() > 0) {
            _coeffVar = stddev() / average();
        } else {
            _coeffVar = 0;
        }

        calculated_coeffVar = true;
    }

    void calculate_variance() {
        long double tot = 0;
        long double tot_prev = 0;
        long double sumsq = 0;
        long double sumsq_prev = 0;
        long double n_samples = (long double) _samples->size();
        for (long double val: *_samples) {
            tot_prev = tot;
            tot += val;
            if (val > 0 && tot <= tot_prev) {
                std::cout << "calc_mix: overflow in calculated_variance tot" << std::endl;
	    	std::cout << "tot: "  << tot << "tot_prev: " << tot_prev << "val: " << val << std::endl;        
	    }
            sumsq_prev = sumsq;
            sumsq += (long double) val * val;
            if (val > 0 && sumsq <= sumsq_prev) {
                std::cout << "calc_mix: overflow in calculated_variance sumsq" << std::endl;
            }
        }

        _variance = NAN;

        long double tot_sq = tot * tot;
        if (tot_sq < tot && tot > 1) {
            std::cout << "calc_mix: overflow in calculated_variance tot_sq: " << tot_sq << " tot: " << tot << std::endl;
        }
        if (n_samples > 1) {
            _variance = ((n_samples * sumsq) - (tot_sq)) / (n_samples * (n_samples - 1));
        }

        _average = tot / n_samples;
        calculated_variance = true;
    }

   
};

struct Results {
    Statistics *rate_a;
    Statistics *rate_b;
    Statistics *win_a;
    Statistics *win_b;
    Statistics *qs_a;
    Statistics *qs_b;
    Statistics *drops_qs_a;
    Statistics *drops_qs_b;
    Statistics *marks_a;
    Statistics *marks_b;
    Statistics *util_a;
    Statistics *util_b;
    Statistics *util; // total utilization
    std::vector<Statistics*> allstat;

    double rr_static;
    double wr_static;

    double a_avg;
    double b_avg;

    Results() {
        rate_a = new Statistics("stat_rpf_a"); allstat.push_back(rate_a);
        rate_b = new Statistics("stat_rpf_b"); allstat.push_back(rate_b);
        win_a = new Statistics("stat_win_a"); allstat.push_back(win_a);
        win_b = new Statistics("stat_win_b"); allstat.push_back(win_b);
        qs_a = new Statistics("stat_qs_a"); allstat.push_back(qs_a);
        qs_b = new Statistics("stat_qs_b"); allstat.push_back(qs_b);
        drops_qs_a = new Statistics("stat_drops_a"); allstat.push_back(drops_qs_a);
        drops_qs_b = new Statistics("stat_drops_b"); allstat.push_back(drops_qs_b);
        marks_a = new Statistics("stat_marks_a"); allstat.push_back(marks_a);
	marks_b = new Statistics("stat_marks_b"); allstat.push_back(marks_b);
        util_a = new Statistics("stat_util_a"); allstat.push_back(util_a);
        util_b = new Statistics("stat_util_b"); allstat.push_back(util_b);
        util = new Statistics("stat_util"); allstat.push_back(util);

        rr_static = NAN;
        wr_static = NAN;

        a_avg = NAN;
        b_avg = NAN;
    }
};

struct Parameters *params = new Parameters();
struct Results *res = new Results();

void usage(int argc, char* argv[]) {
    printf("Usage: %s <folder> <link b/s> <rtt_a> <rtt_b> <nr a flows> <nr b flows>\n", argv[0]);
    exit(1);
}

std::ofstream* openFileW(std::string filename) {
    std::string filename_out = params->folder + "/" + filename;
    std::ofstream *f;
    f = new std::ofstream(filename_out.c_str());

    if (!f->is_open()) {
        std::cerr << "error opening file: " << filename_out << std::endl;
        exit(1);
    }

    return f;
}

void writeToFile(Parameters *param, Statistics *s) {
    std::stringstream data;
   // data << "# num_flows average p1 p25 p75 p99 stddev" << std::endl;
    uint64_t num_flows = 0;
    if (0 == s->filename_out.compare(s->filename_out.length() - 7, 7, "_b"))
        num_flows = param->n_b;
    else if (0 == s->filename_out.compare(s->filename_out.length() - 4, 4, "_a"))
        num_flows = param->n_a;
    else
        num_flows = param->n_a + param->n_b;
    data << "s" << num_flows <<  " " << s->average() << " " << s->p(1) << " " << s->p(25) << " " << s->p(75) << " " << s->p(99) << " " << s->stddev() << std::endl;
   
    std::ofstream *fs = openFileW(s->filename_out);
    *fs << data.str();
    fs->close();
}


void dmPDF(Statistics *drops_a, Statistics *drops_b, Statistics *marks_a, Statistics *marks_b, int i) {
    std::ofstream *f_da_pdf = openFileW("d_pf_a_pdf");
    std::ofstream *f_ma_pdf = openFileW("m_pf_a_pdf");
    std::ofstream *f_db_pdf = openFileW("d_pf_b_pdf");
    std::ofstream *f_mb_pdf = openFileW("m_pf_b_pdf");

    std::vector<double> *samples_drops_a = drops_a->samples();
    std::vector<double> *samples_drops_b = drops_b->samples();
    std::vector<double> *samples_marks_a = marks_a->samples();
    std::vector<double> *samples_marks_b = marks_b->samples();

    uint32_t da_pdf[PDF_BINS];
    uint32_t db_pdf[PDF_BINS];
    uint32_t ma_pdf[PDF_BINS];
    uint32_t mb_pdf[PDF_BINS];

    bzero(da_pdf, sizeof(uint32_t)*PDF_BINS);
    bzero(db_pdf, sizeof(uint32_t)*PDF_BINS);
    bzero(ma_pdf, sizeof(uint32_t)*PDF_BINS);
    bzero(mb_pdf, sizeof(uint32_t)*PDF_BINS);


    uint32_t max = 0;

    if (samples_drops_a->back() > max)
        max = samples_drops_a->back();
    if (samples_drops_b->back() > max)
        max = samples_drops_b->back();

    uint32_t binsize = max/PDF_BINS;
    uint32_t b;

    for (double val: *samples_drops_a) {
        b = val / binsize;
        if (b >= PDF_BINS)
            b = PDF_BINS - 1;
        da_pdf[b]++;
    }

    for (double val: *samples_drops_b) {
        b = val / binsize;
        if (b >= PDF_BINS)
            b = PDF_BINS - 1;
        db_pdf[b]++;
    }

    if (samples_marks_a->back() > max)
        max = samples_marks_a->back();
    if (samples_marks_b->back() > max)
        max = samples_marks_b->back();

    binsize = max/PDF_BINS;
    for (double val: *samples_marks_a) {
        b = val / binsize;
        if (b >= PDF_BINS)
            b = PDF_BINS - 1;
        ma_pdf[b]++;
    }

    for (double val: *samples_marks_b) {
        b = val / binsize;
        if (b >= PDF_BINS)
            b = PDF_BINS - 1;
        mb_pdf[b]++;
    }


    for (int i = 0; i < PDF_BINS; ++i) {
        *f_da_pdf << i << " " << da_pdf[i] << std::endl;
        *f_ma_pdf << i << " " << ma_pdf[i] << std::endl;
        *f_db_pdf << i << " " << db_pdf[i] << std::endl;
        *f_mb_pdf << i << " " << mb_pdf[i] << std::endl;
    }

    f_da_pdf->close();
    f_ma_pdf->close();
    f_db_pdf->close();
    f_mb_pdf->close();
}

void rPDF(Statistics *rate_a, Statistics *rate_b, int n_a, int n_b) {
    std::ofstream *f_ra_pdf = openFileW("r_pf_a_pdf");
    std::ofstream *f_rb_pdf = openFileW("r_pf_b_pdf");
    uint64_t ra_pdf[PDF_BINS];
    uint64_t rb_pdf[PDF_BINS];
    bzero(ra_pdf, sizeof(uint64_t)*PDF_BINS);
    bzero(rb_pdf, sizeof(uint64_t)*PDF_BINS);

    std::vector<double> *samples_rate_a = rate_a->samples();
    std::vector<double> *samples_rate_b = rate_b->samples();

    uint32_t max = n_a + n_b;
    if (max == 0)
        max = 1;

    max = 10000000/max;

    uint32_t binsize = max/PDF_BINS;
    uint32_t b;

    for (double val: *samples_rate_a) {
        b = val / binsize;
        if (b >= PDF_BINS)
            b = PDF_BINS - 1;
        ra_pdf[b]++;
    }

    for (double val: *samples_rate_b) {
        b = val / binsize;
        if (b >= PDF_BINS)
            b = PDF_BINS - 1;
        rb_pdf[b]++;
    }

    for (int i = 0; i < PDF_BINS; ++i) {
        *f_ra_pdf << i << " " << ra_pdf[i] << std::endl;
        *f_rb_pdf << i << " " << rb_pdf[i] << std::endl;
    }

    f_ra_pdf->close();
    f_rb_pdf->close();
}

void readFileMarks(std::string filename_marks, Statistics *stats, std::string filename_tot) {
    std::ifstream infile_marks(filename_marks.c_str());
    std::ifstream infile_tot(filename_tot.c_str());

    double marks;
    double tot_packets;
    std::vector<double> *samples = new std::vector<double>();

    for (int s = 0; s < NRSAMPLES; ++s) {
        for (int colnr = 0; colnr < 3; ++colnr) {
            if (infile_marks.eof() || infile_tot.eof())
                break;

            infile_marks >> marks;

            if (colnr == 2) {
                infile_tot >> tot_packets;
                double marks_perc = 0;

                if (tot_packets > 0) {
                    marks_perc = marks * 100 / tot_packets;
                    samples->push_back(marks_perc);
                }

            }
        }
    }

    infile_marks.close();
    infile_tot.close();
    stats->samples(samples);
}

void readFileDrops(std::string filename_drops, Statistics *stats, std::string filename_tot) {
    std::ifstream infile_drops(filename_drops.c_str());
    std::ifstream infile_tot(filename_tot.c_str());

    double drops;
    double tot_packets;
    std::vector<double> *samples = new std::vector<double>();

    for (int s = 0; s < NRSAMPLES; ++s) {
        for (int colnr = 0; colnr < 3; ++colnr) {
            if (infile_drops.eof() || infile_tot.eof())
                break;

            infile_drops >> drops;

            if (colnr == 2) {
                infile_tot >> tot_packets;
                double drops_perc = 0;

                if (tot_packets+drops > 0) {
                    drops_perc = drops*100/(tot_packets+drops);
                    samples->push_back(drops_perc); 
                    if (drops_perc > 100) {
                        std::cout << "too large drops perc: " << drops_perc << std::endl;
                        std::cout << "tot_packets: " << tot_packets << " drops: " << drops << std::endl;
                    }    
                }
            }
        }
    }

    infile_drops.close();
    infile_tot.close();
    stats->samples(samples);
}

void readFileRate(std::string filename, int nrflows, Statistics *stats_rate, Statistics *stats_win, double avg_qs, double rtt) {
    std::ifstream infile(filename.c_str());
    double rate;

    std::vector<double> *samples_rate = new std::vector<double>();
    std::vector<double> *samples_win = new std::vector<double>();

    for (int s = 0; s < NRSAMPLES; ++s) {
        for(std::string line; getline(infile, line);) {
            std::istringstream iss(line);
            int colnr = 0;

            while (iss >> rate) {
                rate /= 8;
		//std::cout << "rate: " << rate << "col: " << colnr << std::endl;
                if (colnr++ >= 2) {
                    double win = 0;
                    if (avg_qs != 0) {
                        win = rate*(avg_qs+rtt)/1000;
                    }

                    samples_rate->push_back(rate);
                    samples_win->push_back(win);
                }
            }

            for (int i = colnr; i < nrflows; ++i) {
                samples_rate->push_back(0);
                samples_win->push_back(0);
            }
        }
    }

    infile.close();
    stats_rate->samples(samples_rate);
    stats_win->samples(samples_win);
}

void getSamplesUtilization() {
    std::string filename_a = params->folder + "/rate_ecn";
    std::string filename_b = params->folder + "/rate_nonecn";

    std::ifstream infile_a(filename_a.c_str());
    std::ifstream infile_b(filename_b.c_str());

    std::vector<double> *samples_a = new std::vector<double>();
    std::vector<double> *samples_b = new std::vector<double>();
    std::vector<double> *samples = new std::vector<double>();
    double rate_a;
    double rate_b;
    double util_a;
    double util_b;
    double util;
    double link_bytes_ps = (double)params->link*125000;

   
    // each line consists of three numbers, and we only want the last number
    for (int s = 0; s < NRSAMPLES; ++s) {

        if (infile_a.eof() || infile_b.eof()) {
            break;
        }
        for (int colnr = 0; colnr < 3; ++colnr) {
            infile_a >> rate_a;
            infile_b >> rate_b;
            if (colnr == 2) {
                util_a = rate_a/8 * 100 / link_bytes_ps;
                if (util_a > 100) util_a = 100;
                util_b = rate_b/8 * 100 / link_bytes_ps;
                if (util_b > 100) util_b = 100;
                util = (rate_a+rate_b)/8 * 100 / link_bytes_ps;
                if (util > 100) util = 100;
                samples_a->push_back(util_a);
                samples_b->push_back(util_b);
                samples->push_back(util);
            }
        }
    }

    infile_a.close();
    infile_b.close();
    res->util_a->samples(samples_a);
    res->util_b->samples(samples_b);
    res->util->samples(samples);
}

void readFileQS(std::string filename, Statistics *stats) {
    std::ifstream infile(filename.c_str());
    if (!infile.is_open()) {
        std::cerr << "calc_mix: Error opening file for reading: " << filename << std::endl;
        exit(1);
    }

    std::vector<double> *samples = new std::vector<double>();

    // Columns in file we are reading:
    // <queuing delay in us> <number of packes not dropped> <number of packets dropped>

    // we don't skip any samples for this one, as the input data
    // is already aggregated over all samples

    while (1) {
        double us;
        double nrpackets;
        double drops;

        infile >> us; /* number of us each packet represents */
        infile >> nrpackets;
        infile >> drops;

        if (infile.eof()) {
            break;
        }

        for (int i = 0; i < nrpackets; ++i) {
            samples->push_back(us/1000); // push back in ms
        }
    }

    infile.close();

    stats->samples(samples);
}

void getSamplesRateMarksDrops() {
    readFileRate(params->folder + "/flows_rate_ecn", params->n_a, res->rate_a, res->win_a, res->qs_a->average(), params->rtt_a);
    readFileMarks(params->folder + "/marks_ecn", res->marks_a, params->folder + "/packets_ecn");
    readFileDrops(params->folder + "/drops_ecn", res->drops_qs_a, params->folder + "/packets_ecn");
    readFileRate(params->folder + "/flows_rate_nonecn", params->n_b, res->rate_b, res->win_b, res->qs_b->average(), params->rtt_b);
    readFileMarks(params->folder + "/marks_nonecn", res->marks_b, params->folder + "/packets_nonecn");
    readFileDrops(params->folder + "/drops_nonecn", res->drops_qs_b, params->folder + "/packets_nonecn");
}

void getSamplesQS() {
    readFileQS(params->folder + "/queue_packets_drops_a_pdf", res->qs_a);
    readFileQS(params->folder + "/queue_packets_drops_b_pdf", res->qs_b);
}

void loadParameters(int argc, char **argv) {
    if (argc < 7) {
	std::cout << "argc " << argc << std::endl;
        usage(argc, argv);
    }

    params->folder = argv[1];
    params->link = atoi(argv[2]);
    params->rtt_a = (double) atoi(argv[3]);
    params->rtt_b = (double) atoi(argv[4]);
    params->n_a = atoi(argv[5]);
    params->n_b = atoi(argv[6]);
}

int main(int argc, char **argv) {
    loadParameters(argc, argv);

    getSamplesQS();
    getSamplesRateMarksDrops();
    getSamplesUtilization();

    if (params->n_b > 0) {
        res->rr_static = res->rate_a->average() /  res->rate_b->average();
        res->wr_static = res->win_a->average() / res->win_b->average();
    }

    if (res->drops_qs_b->p(99) > 100) {
        std::cerr << "too high drops p99: " << res->drops_qs_b->p(99) << std::endl;
        exit(1);
    }

    for (auto it = res->allstat.begin(); it != res->allstat.end(); ++it) {
        Statistics* stat = *it;
        writeToFile(params, stat);
    }

    std::ofstream *f_avgrate_a = openFileW("avgrate_a");
    if (f_avgrate_a->is_open()){
        *f_avgrate_a << (int)res->rate_a->average();
        f_avgrate_a->close();
    }
    std::ofstream *f_avgrate_b = openFileW("avgrate_b");
    if (f_avgrate_b->is_open()){
        *f_avgrate_b << (int)res->rate_b->average();
        f_avgrate_b->close();
    }

    std::ofstream *f_avgqs_a = openFileW("avgqs_a");
    if (f_avgqs_a->is_open()){
        *f_avgqs_a << (int)res->qs_a->average();
        f_avgqs_a->close();
    }
    std::ofstream *f_avgqs_b = openFileW("avgqs_b");
    if (f_avgqs_b->is_open()){
        *f_avgqs_b << (int)res->qs_b->average();
        f_avgqs_b->close();
    }

    std::ofstream *f_rr = openFileW("rr");
 	*f_rr << res->rr_static;
 	f_rr->close();

    std::ofstream *f_wr = openFileW("wr");
    *f_wr << res->wr_static;
    f_wr->close();

    std::ofstream *f_fairwin = openFileW("fairwin");
    double fairwin = params->link*125000/(params->n_a/(res->qs_a->average()+params->rtt_a) + params->n_b/(res->qs_b->average()+params->rtt_b))/1000;
    *f_fairwin << fairwin;
    f_fairwin->close();



    return 0;
}
