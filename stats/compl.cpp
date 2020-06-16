#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <fstream>
#include <vector>
#include <math.h>
#include <sstream>
#include <string>
#include <unistd.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <string.h>
#include <algorithm>

#define percentile(p, n) (round(float(p)/100*float(n) + float(1)/2))

struct BytesTime {
    double bytes;
    double time_usec;
    double time_norm;
    BytesTime(uint32_t b, uint32_t t, double o) {
        bytes = (double) b;
        time_usec = (double) t;
        time_norm = o;
    }
    static bool compareBytes(BytesTime lhs, BytesTime rhs) {
        return lhs.bytes < rhs.bytes;
    }
    static bool compareTimeNorm(BytesTime lhs, BytesTime rhs) {
        return lhs.time_norm < rhs.time_norm;
    }

};

struct ComplStat {
    std::vector<BytesTime> compl_time;

    std::vector<double> ct_norm_avg;
    std::vector<double> ct_opt;
    std::vector<double> bin_avg_size;

    std::vector<uint32_t> ct_nr_s;
    std::vector<double> ct_norm_p99;
    std::vector<double> ct_norm_p1;

    std::vector<BytesTime> samples;

    uint32_t m_rtt;
    uint32_t m_rate;
    std::string m_foldername;
    std::string asupport;
    std::ofstream *outfile;

    ComplStat(uint32_t nr_bins_norm, uint32_t rtt, uint32_t rate, std::string foldername, std::string a) 
    {
        m_rtt = rtt;
        m_rate = rate;
        m_foldername = foldername;
        asupport = a;
        ct_norm_avg.resize(nr_bins_norm);
        ct_opt.resize(nr_bins_norm);
        bin_avg_size.resize(nr_bins_norm);
        ct_nr_s.resize(nr_bins_norm);
        ct_norm_p99.resize(nr_bins_norm);
        ct_norm_p1.resize(nr_bins_norm);

        std::fill(ct_norm_avg.begin(), ct_norm_avg.end(), 0);
        std::fill(ct_opt.begin(), ct_opt.end(), 0);
        std::fill(bin_avg_size.begin(), bin_avg_size.end(), 0);
        std::fill(ct_nr_s.begin(), ct_nr_s.end(), 0);
        std::fill(ct_norm_p99.begin(), ct_norm_p99.end(), 0);
        std::fill(ct_norm_p1.begin(), ct_norm_p1.end(), 0);

        std::stringstream filename;
        filename << m_foldername << "/bins_" << a;
        outfile = new std::ofstream(filename.str().c_str());
        if (!outfile->is_open()) {
            std::cerr << "eror opening file: " << filename.str() << std::endl;
        }
     }
};

std::vector<double> size_bins;



void usage(int argc, char* argv[])
{
    printf("Usage: %s <folder> <avgrate_a> <avgrate_b> <rtt> <link>\n", argv[0]);
    exit(1);
}

double twoDecimalPlaces(double number) {
    uint32_t scaled = (uint32_t)round(number * 100);
    return ((double)number)/100;
}

double getOptComplTimeUs(uint32_t rtt, double bytes, uint32_t rate) {
    double rtt_sec = ((double)rtt)/1000;
    return ((bytes/rate + rtt_sec * 2) * 1000000);
}

void outputBinToFile(ComplStat *cs, uint32_t index) {
    for (std::vector<BytesTime>::iterator it = cs->samples.begin(); it != cs->samples.end(); ++it) {
        *cs->outfile << "bin" << index << " " << it->bytes << " " << it->time_usec << " " << it->time_norm << std::endl;
    }
}

void processBin(ComplStat *cs, uint32_t index) {
    if (cs->ct_nr_s.at(index) > 0) {
        cs->ct_norm_avg.at(index) /= cs->ct_nr_s.at(index);
        outputBinToFile(cs, index);
        std::sort(cs->samples.begin(), cs->samples.end(), BytesTime::compareTimeNorm);
        uint32_t p99_index = (uint32_t)percentile(99, cs->ct_nr_s.at(index)) - 1;
        uint32_t p1_index = (uint32_t)percentile(1, cs->ct_nr_s.at(index)) - 1;
        cs->ct_norm_p99.at(index) = cs->samples.at(p99_index).time_norm;
        cs->ct_norm_p1.at(index) = cs->samples.at(p1_index).time_norm;
        cs->samples.clear();
        int nr_samples = cs->compl_time.size();
        if (p99_index >= cs->ct_nr_s.at(index) || p99_index >= nr_samples || p1_index >= nr_samples){
            std::cerr << "wrong index: p99: " << p99_index << " p1: " << p1_index << "nr_samples_total:" << 
            nr_samples << "nr_samples_bin" << cs->ct_nr_s.at(index) << "size of samples " << cs->samples.size() << std::endl; 
            exit(1);
        }
    }
}

void readFromFile(std::string filename_in, ComplStat *cs) {
    std::ifstream infile(filename_in.c_str());
    if (!infile.is_open()) {
        std::cerr << "eror opening file: " << filename_in << std::endl;
        return;
    }
    double b, t;

    while (!infile.eof()) {
        infile >> b;
        infile >> t;
        if (b >= 1000) {
            double oct_sample = getOptComplTimeUs(cs->m_rtt, b, cs->m_rate) / t;
            cs->compl_time.push_back(BytesTime(b,t,oct_sample));
        }
       
    }
    infile.close();
}

void processFile(std::string folder, std::string fn_in, std::string fn_out, std::string fn_bins, uint32_t rtt, double link_bytes_ps, double avg_rate_pf, std::string a)
{    
    uint32_t nr_bins_norm = size_bins.size();
    ComplStat *complstat = new ComplStat(nr_bins_norm, rtt, link_bytes_ps, folder, a);

    if (avg_rate_pf == 0) {
        std::cerr << "ERROR: avg rate is 0, setting to 1 to avoid division by 0" << std::endl;
        avg_rate_pf = 1;
    }
   
    std::string filename_in = folder + fn_in;
    readFromFile(filename_in, complstat);
    
    // Normalised completion time using bins 
    std::string filename_out_bins = folder + fn_bins;
    std::ofstream outfile_bins(filename_out_bins.c_str());
    if (!outfile_bins.is_open())
        std::cerr << "error opening file to write completion bins: " << filename_out_bins << std::endl;

    std::sort(complstat->compl_time.begin(), complstat->compl_time.end(), BytesTime::compareBytes);
    
    std::vector<BytesTime>::iterator it = complstat->compl_time.begin();
    int index = 0;
    while (index < nr_bins_norm) {
        double lower_limit = 1000;
        if (index > 0)
            lower_limit = size_bins.at(index-1);
        double upper_limit = size_bins.at(index);

        complstat->bin_avg_size.at(index) = sqrt(upper_limit*lower_limit); //log average
        complstat->ct_opt.at(index) = getOptComplTimeUs(rtt, complstat->bin_avg_size.at(index), link_bytes_ps);
    
        while (it != complstat->compl_time.end() && it->bytes < upper_limit) { 
            complstat->ct_norm_avg.at(index) += it->time_norm;
            complstat->samples.push_back(*it); 
            complstat->ct_nr_s.at(index)++;
            it++;
        }        

        processBin(complstat, index);
        index++;
    }

    for (int i = 0; i < nr_bins_norm; ++i) {
        if (complstat->ct_nr_s.at(i) > 0 && complstat->bin_avg_size.at(i) > 0) {
            double size;
            if (i == 0)
                size = 1;
            else
                size = size_bins.at(i-1)/1000.0;
            outfile_bins << size << " " << 
            complstat->ct_norm_avg.at(i) << " " << 
            complstat->ct_norm_p99.at(i) << " " << 
            complstat->ct_norm_p1.at(i) << " " << 
            complstat->ct_opt.at(i) << " " << 
            complstat->ct_nr_s.at(i) << std::endl;
        }
    }
    outfile_bins.close();
    complstat->outfile->close();

    
}

// transfer size/ratein bytes per second/ + 2rtt
int main(int argc, char **argv)
{
	if (argc < 6)
		usage(argc,argv);

    std::string folder = argv[1];
    double avgrate_a = atof(argv[2]);
    double avgrate_b = atof(argv[3]);
    uint32_t rtt = atoi(argv[4]);
    uint32_t link = atoi(argv[5]);

    double link_bytes_ps = link*1000000/8;

    double bytes = 1000;
    double power = 1;
    while (bytes < 1000000.0) {
        bytes = 1000*pow(3,power++);
        if (bytes > 1000000)
            bytes = 1000000;
       // std::cout << bytes  << std::endl;
        size_bins.push_back(bytes);
    }

    processFile(folder, "/completion_time_ecn_w_hs", "/compl_a_w_hs", "/compl_bins_a_hs", rtt, link_bytes_ps, avgrate_a, "a");
    processFile(folder, "/completion_time_nonecn_w_hs", "/compl_b_w_hs", "/compl_bins_b_hs", rtt, link_bytes_ps, avgrate_b, "b");
    
    return 0;
}

