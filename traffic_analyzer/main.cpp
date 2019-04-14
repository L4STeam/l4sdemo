#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "analyzer.h"

void usage(int argc, char* argv[])
{
    printf("Usage: %s <dev> <pcap filter exp> <output folder> <sample interval (ms)> <ipclass> [nrsamples]\n", argv[0]);
    printf("pcap filter: what to capture. ex.: \"ip and src net 10.187.255.0/24\"\n");
    printf("If nrsamples is not specified, the samples will be recorded until interrupted\n");
    exit(1);
}

int main(int argc, char **argv)
{
    char *dev;
    uint32_t sinterval;
    bool ipclass = false;
    uint32_t nrs = 0;


    if (argc < 5)
        usage(argc, argv);

    dev = argv[1];

    std::string pcapfilter = argv[2];
    std::string folder = argv[3];
    sinterval = atoi(argv[4]);

    std::cout << "pcap filter: " << pcapfilter << std::endl;

    if (argc > 5)
        ipclass = (argv[5][0] == 't');

    if (argc > 6)
        nrs = atoi(argv[6]);

    mkdir(folder.c_str(), 0777);

    ThreadParam *param = new ThreadParam(sinterval, folder, ipclass, nrs); 

    setup_pcap(param, dev, pcapfilter);
    start_analysis(param);

    return 0;
}
