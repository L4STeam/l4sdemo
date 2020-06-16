#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include <cmath>
#include <algorithm>

#define percentile(p, n) (ceil(float(p)/100*float(n)))

#define NR_SAMPLES 250

void openFileR(std::ifstream& file, std::string filename) {
    file.open(filename.c_str());
    if (!file.is_open()) {
        std::cerr << "calc_queue_packets_drops: Error opening file for reading: " << filename << std::endl;
        exit(1);
    }
}

void openFileW(std::ofstream& file, std::string filename) {
    file.open(filename.c_str());
    if (!file.is_open()) {
        std::cerr << "calc_queue_packets_drops: Error opening file for writing: " << filename << std::endl;
        exit(1);
    }
}

void readFile(std::string filename, std::vector<uint64_t> **header, std::vector<uint64_t> **sample_time,
    std::vector<uint64_t> **values, std::vector<std::vector<uint64_t>> **values_timeline, int samples_to_skip)
{
    std::ifstream infile;
    openFileR(infile, filename);

    // Format of file:
    // Header row: <number of columns> <value 1 header> <value 2 header> ...
    // Each following row is a sample: <sample time> <value 1> <value 2> ...

    // Read header
    int column_count = 0;
    infile >> column_count;

    for (int i = 0; i < column_count; i++) {
        uint64_t value;
        infile >> value;
        if (sample_time != NULL) {
            (*header)->push_back(value);
        }
    }

    // Skip samples we are not interested in
    std::string line;
    for (int i = 0; i < samples_to_skip; i++) {
        getline(infile, line);
    }

    // Read samples and aggregate the rows
    if (*values == NULL) {
        *values = new std::vector<uint64_t>(column_count, 0);
    }


    int samples = 0;
    while (1) {
        uint64_t value;
        uint64_t qdelay;
        uint64_t value_ms;
        // Skip first col
        infile >> value_ms;

        if (infile.eof()) {
            break;
        }
        if (sample_time != NULL) {
            (*sample_time)->push_back(value_ms);
        }

        for (int i = 0; i < column_count; i++) {
            infile >> value;
            if (value > 0) {
                (*values)->at(i) += value;
                qdelay = (*header)->at(i);
                (*values_timeline)->at(samples).push_back(qdelay);
            }
        }
        samples++;
    }
}

void writePdfCdf(std::string filename_pdf, std::string filename_cdf, std::string filename_ccdf, std::vector<uint64_t> *header, std::vector<uint64_t> *sent, std::vector<uint64_t> *drops) {
    std::ofstream f_pdf, f_cdf, f_ccdf;
    openFileW(f_pdf, filename_pdf);
    openFileW(f_cdf, filename_cdf);
    openFileW(f_ccdf, filename_ccdf);

    // Columns in the output:
    // (one row for each queue delay)
    // <queue delay> <number of packets sent> <number of packets dropped>

    uint64_t sent_cdf = 0;
    long double sent_ccdf = 0;
    long double sent_ccdf_prev = 0;
    uint64_t drops_cdf = 0;
    uint64_t nr_samples = 0;


    auto it_header = begin(*header);
    auto it_sent = begin(*sent);

    auto it_drops = begin(*drops);

    while (it_sent != end(*sent)) {
        nr_samples += *it_sent;
        it_sent++;
    }
    
    it_sent = begin(*sent);

    while (it_header != end(*header)) {
        sent_cdf += *it_sent;
        if (*it_sent > 0) {
            sent_ccdf = *it_sent * 100.0 / nr_samples + sent_ccdf_prev;
            sent_ccdf_prev = sent_ccdf;

            sent_ccdf = 100.0 - sent_ccdf;
            f_ccdf << *it_header << " " << sent_ccdf << " "  << std::endl;

        }
        drops_cdf += *it_drops;

        f_pdf << *it_header << " " << *it_sent << " " << *it_drops << std::endl;
        f_cdf << *it_header << " " << sent_cdf << " " << drops_cdf << std::endl;


        it_header++;
        it_sent++;
        it_drops++;
    }


    f_pdf.close();
    f_cdf.close();
    f_ccdf.close();

}

void writeTimeline(std::string filename_timeline, std::vector<std::vector<uint64_t>> *samples, std::vector<uint64_t> *sample_time) {
    std::ofstream f_timeline;
    openFileW(f_timeline, filename_timeline);

    // Columns in the output:
    // (one row for each queue delay)
    // <sample time> <avg queue delay>
    auto it_sample_time = begin(*sample_time);
    auto it_samples = begin(*samples);

    while (it_sample_time != end(*sample_time)) {
        std::sort(it_samples->begin(), it_samples->end());

        uint64_t sum = 0;
        uint64_t size = it_samples->size();
        uint64_t diff = 0;
        long double avg = 0;
        if (size > 0) {
            diff = it_samples->at(size-1) - it_samples->at(0);
            for (auto it_values = begin(*it_samples); it_values != end(*it_samples); it_values++)
                sum += *it_values;
            avg = (long double)sum/size;
        }

        f_timeline << *it_sample_time << " " << avg << " " << diff << std::endl;

        it_sample_time++;
        it_samples++;
    }

    f_timeline.close();
}


void usage(int argc, char* argv[]) {
    printf("Usage: %s <test_folder> <samples_to_skip>\n", argv[0]);
    exit(1);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        usage(argc, argv);
    }

    std::string folder = argv[1];
    int samples_to_skip = atoi(argv[2]);

    std::vector<uint64_t> *header = new std::vector<uint64_t>();
    std::vector<uint64_t> *sample_time = new std::vector<uint64_t>();
    std::vector<uint64_t> *values_qd_classic = NULL;
    std::vector<uint64_t> *values_qd_l4s = NULL;

    std::vector<std::vector<uint64_t>> *values_qd_l4s_timeline = new std::vector<std::vector<uint64_t>>(NR_SAMPLES);
    std::vector<std::vector<uint64_t>> *values_qd_classic_timeline = new std::vector<std::vector<uint64_t>>(NR_SAMPLES);


    std::vector<uint64_t> *values_d_classic = NULL;
    std::vector<uint64_t> *values_d_l4s = NULL;

    std::vector<std::vector<uint64_t>> *values_d_classic_timeline = new std::vector<std::vector<uint64_t>>(NR_SAMPLES);
    std::vector<std::vector<uint64_t>> *values_d_l4s_timeline = new std::vector<std::vector<uint64_t>>(NR_SAMPLES);


    readFile(folder + "/queue_packets_ecn00", &header, &sample_time, &values_qd_classic, &values_qd_classic_timeline, samples_to_skip);
    readFile(folder + "/queue_packets_ecn01", &header, NULL, &values_qd_l4s, &values_qd_l4s_timeline, samples_to_skip);
    readFile(folder + "/queue_packets_ecn10", &header, NULL, &values_qd_l4s, &values_qd_l4s_timeline, samples_to_skip);
    readFile(folder + "/queue_packets_ecn11", &header, NULL, &values_qd_l4s, &values_qd_l4s_timeline, samples_to_skip);

    readFile(folder + "/queue_drops_ecn00", &header, NULL, &values_d_classic, &values_d_classic_timeline, samples_to_skip);
    readFile(folder + "/queue_drops_ecn01", &header, NULL, &values_d_l4s, &values_d_l4s_timeline, samples_to_skip);
    readFile(folder + "/queue_drops_ecn10", &header, NULL, &values_d_l4s, &values_d_l4s_timeline, samples_to_skip);
    readFile(folder + "/queue_drops_ecn11", &header, NULL, &values_d_l4s, &values_d_l4s_timeline, samples_to_skip);


    writePdfCdf(
        folder + "/queue_packets_drops_b_pdf",
        folder + "/queue_packets_drops_b_cdf",
        folder + "/queue_delay_b_ccdf",
        header,
        values_qd_classic,
        values_d_classic
    );

    writePdfCdf(
        folder + "/queue_packets_drops_a_pdf",
        folder + "/queue_packets_drops_a_cdf",
        folder + "/queue_delay_a_ccdf",
        header,
        values_qd_l4s,
        values_d_l4s
    );

    writeTimeline(
        folder + "/queue_delay_timeline_ecn",
        values_qd_l4s_timeline,
        sample_time
    );

    writeTimeline(
        folder + "/queue_delay_timeline_nonecn",
        values_qd_classic_timeline,
        sample_time
    );
}
