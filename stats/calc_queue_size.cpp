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
        std::cerr << "calc_queue_size: Error opening file for reading: " << filename << std::endl;
        exit(1);
    }
}

void openFileW(std::ofstream& file, std::string filename) {
    file.open(filename.c_str());
    if (!file.is_open()) {
        std::cerr << "calc_queue_size: Error opening file for writing: " << filename << std::endl;
        exit(1);
    }
}

void readFile(std::string filename, std::vector<uint64_t> **header,
    std::vector<uint64_t> **values)
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
        if ((*header)->size() < column_count)
            (*header)->push_back(value);
	}

    // Read samples and aggregate the rows
    if (*values == NULL) {
        *values = new std::vector<uint64_t>(column_count, 0);
    }

    int samples = 0;
    while (1) {
        uint64_t value;
        uint64_t qsize;
        uint64_t value_ms;
        // Skip first col
        infile >> value_ms;


        if (infile.eof()) {
            break;
        }

        for (int i = 0; i < column_count; i++) {
            infile >> value;

            if (value > 0) {
                (*values)->at(i) += value;
            }
        }

        samples++;
    }

}

void writePdfCdf(std::string filename_pdf, std::string filename_cdf, std::string filename_ccdf, std::string filename_nrs, std::vector<uint64_t> *header, std::vector<uint64_t> *qs) {

    std::ofstream f_ccdf, f_cdf, f_nrs;
    openFileW(f_cdf, filename_cdf);
    openFileW(f_ccdf, filename_ccdf);
    openFileW(f_nrs, filename_nrs);


    uint64_t qs_cdf = 0;
    long double qs_ccdf = 0;
    long double qs_ccdf_prev = 0;
    uint64_t nr_samples = 0;


    auto it_header = begin(*header);
    auto it_qs = begin(*qs);


    while (it_qs != end(*qs)) {
        nr_samples += *it_qs;
        it_qs++;
    }

    f_nrs << nr_samples << std::endl;

    it_qs = begin(*qs);

    while (it_header != end(*header)) {
        qs_cdf += *it_qs;
        if (*it_qs > 0) {

            qs_ccdf = *it_qs * 100.0 / nr_samples + qs_ccdf_prev;
            qs_ccdf_prev = qs_ccdf;
            qs_ccdf = 100 - qs_ccdf;
        }

        f_ccdf << *it_header << " " << qs_ccdf  << " "  << std::endl;
        f_cdf << *it_header << " " << qs_cdf * 100.0 / nr_samples << " " << qs_cdf << std::endl;


        it_header++;
        it_qs++;

    }

    f_ccdf.close();
    f_cdf.close();
    f_nrs.close();

}



void usage(int argc, char* argv[]) {
    printf("Usage: %s <test_folder> <samples_to_skip>\n", argv[0]);
    exit(1);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        usage(argc, argv);
    }

    std::string folder = argv[1];

    std::vector<uint64_t> *header = new std::vector<uint64_t>();
    std::vector<uint64_t> *values_qs_classic = NULL;
    std::vector<uint64_t> *values_qs_l4s = NULL;

    readFile(folder + "/qs_l", &header, &values_qs_l4s);
    readFile(folder + "/qs_c", &header, &values_qs_classic);

    writePdfCdf(
        folder + "/qs_l_pdf",
        folder + "/qs_l_cdf",
        folder + "/qs_l_ccdf",
        folder + "/nrs_l",
        header,
        values_qs_l4s
    );

    writePdfCdf(
        folder + "/qs_c_pdf",
        folder + "/qs_c_cdf",
        folder + "/qs_c_ccdf",
        folder + "/nrs_c",
        header,
        values_qs_classic
    );

}
