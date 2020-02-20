#include "analyzer.h"

#include <csignal>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <iostream>
#include <map>
#include <unistd.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <strings.h>
#include <fstream>
#include <vector>
#include <math.h>
#include <array>
#include <time.h>
#include <sys/types.h>

typedef u_int32_t u32; // we use "kernel-style" u32 variables in numbers.h
#define TESTBED_ANALYZER 1
#include "../common/numbers.h"

#define NSEC_PER_SEC 1000000000UL
#define NSEC_PER_MS 1000000UL
#define US_PER_S 1000000UL
#define NSEC_PER_US 1000UL

enum {
	INET_ECN_NOT_ECT = 0,
	INET_ECN_ECT_1 = 1,
	INET_ECN_ECT_0 = 2,
	INET_ECN_CE = 3,
	INET_ECN_MASK = 3,
};

struct vlan_header {
    struct ether_header ether;
    __be16 vlan_tag;
    __be16 ether_type;
} __attribute__((__packed__));

static void *printInfo(void *param);

uint64_t getStamp()
{
    // returns us
    struct timespec monotime;
    clock_gettime(CLOCK_MONOTONIC, &monotime);
    return ((uint64_t)monotime.tv_sec) * US_PER_S + monotime.tv_nsec / NSEC_PER_US;
}

ThreadParam::ThreadParam(uint32_t sinterval, std::string folder,
			 bool ipc, uint32_t nrs)
	: m_sinterval(sinterval)
	, m_folder(folder)
	, ipclass(ipc)
	, m_nrs(nrs)
	, quiet(false)
{
    // initialize qdelay conversion table
    for (int i = 0; i < QS_LIMIT; ++i) {
        qdelay_decode_table[i] = qdelay_decode(i);
    }

    db1 = new DataBlock();
    db2 = new DataBlock();
    db1->init();
    db1->start = getStamp();

    pthread_mutexattr_t errorcheck;
    pthread_mutexattr_init(&errorcheck);
    pthread_mutex_init(&m_mutex, &errorcheck);

    packets_captured = 0;
    packets_processed = 0;

    quit = false;
    sample_id = 0;

    // initialize pthread condition used to quit threads on interrupts
    pthread_condattr_t attr;
    pthread_condattr_init(&attr);
    pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
    pthread_cond_init(&quit_cond, &attr);
    pthread_mutex_init(&quit_lock, NULL);
}

void ThreadParam::swapDB(){ // called by printInfo
    // db2 is initialized already
    pthread_mutex_lock(&m_mutex);
    DataBlock *tmp = db1;
    db1 = db2;
    db2->start = getStamp();
    tmp->last = db1->start;
    pthread_mutex_unlock(&m_mutex);
    db2 = tmp;
}

// we need ThreadParam global to use it in the signal handler
static ThreadParam *tp;

void signalHandler(int signum) {
    tp->quit = true;
    pthread_cond_broadcast(&tp->quit_cond);
}

std::string IPtoString(in_addr_t ip) {
    struct sockaddr_in ipip;
    ipip.sin_addr.s_addr = ip;
    return std::string(inet_ntoa(ipip.sin_addr));
}

// Queuing delay is returned in ns
int decodeQdelay(u32 value) {
    return qdelay_decode(value);
}

int decodeDrops(u32 value) {
    return fl2int(value, DROPS_M, DROPS_E);
}

void processPacket(u_char *, const struct pcap_pkthdr *header,
		   const u_char *buffer)
{
#define _PARSE(type, name, offset) type *name = (type *)(buffer + offset)
#define PARSE(type, name) _PARSE(type, name, offset)

    uint32_t mark;
    int offset, drops, qdelay_encoded;
    uint32_t saddr, daddr; /* Will need to make this more generic */
    uint16_t sport, dport;
    uint8_t ts, proto;
    double bitlen = header->len << 3;

{
    PARSE(struct ether_header, eth);
    offset = sizeof(*eth);
    __be16 type = eth->ether_type;
parse_ethernet:
    switch (ntohs(type)) {
    case ETH_P_8021Q: {
        if (offset != sizeof(*eth)) {
            std::cerr << "Ignoring nested VLANs" << std::endl;
            return;
        }
        _PARSE(struct vlan_header, vlan, 0);
        offset = sizeof(*vlan);
        type = vlan->ether_type;
        goto parse_ethernet;
    }
    case ETH_P_IP: goto parse_ip;
    case ETH_P_IPV6: goto parse_ipv6;
    default: return;
    }
}

parse_ip: {
    PARSE(struct iphdr, iph);
    offset += (iph->ihl << 2);
    proto = iph->protocol;
    saddr = iph->saddr;
    daddr = iph->daddr;
    ts = iph->tos & INET_ECN_MASK;
    mark = (ts == INET_ECN_CE);
    if (tp->ipclass)
        ts = ntohl(saddr);
    uint16_t id = ntohs(iph->id);
    drops = decodeDrops(id >> 11); // drops stored in 5 bits MSB
    // We don't decode queueing delay here as we need to store it in a table,
    // so defer this to the actual serialization of the table to file
    qdelay_encoded = id & 2047; // 2047 = 0b0000011111111111
    goto parse_transport;
}

parse_ipv6: {
    /* Parse ipv6 header chains, ... */
    return; /* This is a shame ... */
}

parse_transport: {
    switch (proto) {
    case IPPROTO_TCP: {
        PARSE(struct tcphdr, tcph);
        sport = ntohs(tcph->source);
        dport = ntohs(tcph->dest);
	break;
    }
    case IPPROTO_UDP: {
        PARSE(struct udphdr, udph);
        sport = ntohs(udph->source);
        dport = ntohs(udph->dest);
	break;
    }
    default:
	sport = 0;
	dport = 0;
	break;
    }
}

    SrcDst sd(proto, saddr, sport, daddr, dport);

    pthread_mutex_lock(&tp->m_mutex);

    std::map<SrcDst,FlowData> *fmap;
    switch (ts & INET_ECN_MASK) {
    case INET_ECN_NOT_ECT:
        tp->db1->tot_packets_nonecn++;
        tp->db1->qs.ecn00[qdelay_encoded]++;
        tp->db1->d_qs.ecn00[qdelay_encoded]+= drops;
        fmap = &tp->db1->fm.nonecn_rate;
        break;
    case INET_ECN_ECT_1:
        tp->db1->tot_packets_ecn++;
        tp->db1->qs.ecn01[qdelay_encoded]++;
        tp->db1->d_qs.ecn01[qdelay_encoded]+= drops;
        fmap = &tp->db1->fm.ecn_rate;
        break;
    case INET_ECN_ECT_0:
        tp->db1->tot_packets_ecn++;
        tp->db1->qs.ecn10[qdelay_encoded]++;
        tp->db1->d_qs.ecn10[qdelay_encoded]+= drops;
        fmap = &tp->db1->fm.ecn_rate;
        break;
    case INET_ECN_CE:
        tp->db1->tot_packets_ecn++;
        tp->db1->qs.ecn11[qdelay_encoded]++;
        tp->db1->d_qs.ecn11[qdelay_encoded]+= drops;
        fmap = &tp->db1->fm.ecn_rate;
        break;
    }
    auto ret = fmap->insert(
        std::pair<SrcDst,FlowData>(sd, FlowData(bitlen, drops, mark)));
    if (ret.second == false) /* We already have entries for that flow */
        fmap->at(sd).update(bitlen, drops, mark);
    tp->packets_captured++;

    pthread_mutex_unlock(&tp->m_mutex);
}

void openFileW(std::ofstream& file, std::string filename) {
    file.open(filename.c_str());
    if (!file.is_open()) {
        std::cerr << "Error opening file for writing: " << filename << std::endl;
        exit(1);
    }
}

std::string getProtoRepr(uint8_t proto) {
    if (proto == IPPROTO_TCP)
        return "TCP";
    else if (proto == IPPROTO_UDP)
        return "UDP";
    else if (proto == IPPROTO_ICMP)
        return "ICMP";
    return "UNKNOWN";
}

void printStreamInfo(SrcDst sd)
{
    std::cout
	    << getProtoRepr(sd.m_proto) << " "
	    << IPtoString(sd.m_srcip) << ":" << sd.m_srcport << " -> "
	    << IPtoString(sd.m_dstip) << ":" << sd.m_dstport;
}

int setup_pcap(ThreadParam *param, const char *dev, std::string &pcapfilter)
{
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *descr;
    struct bpf_program fp;      // The compiled filter expression
    bpf_u_int32 mask;       // The netmask of our sniffing device
    bpf_u_int32 net;        // The IP of our sniffing device

    if (pcap_lookupnet(dev, &net, &mask, errbuf) == -1) {
        fprintf(stderr, "Can't get netmask for device %s\n", dev);
        net = 0;
        mask = 0;
    }

    param->m_descr = pcap_open_live(dev, BUFSIZ, 0, 1, errbuf);

    if (param->m_descr == NULL) {
        printf("pcap_open_live(): %s\n", errbuf);
        exit(1);
    }

    if (pcap_compile(param->m_descr, &fp, pcapfilter.c_str(), 0, net) == -1) {
        fprintf(stderr, "Couldn't parse filter: %s\n",
		pcap_geterr(param->m_descr));
        return(2);
    }

    if (pcap_setfilter(param->m_descr, &fp) == -1) {
        fprintf(stderr, "Couldn't install filter: %s\n",
		pcap_geterr(param->m_descr));
        return(2);
    }
    return 0;
}

void setThreadParam(ThreadParam *param)
{
    tp = param;
}

int start_analysis(ThreadParam *param)
{
    pthread_t thread_id[2];
    pthread_attr_t attrs;
    pthread_attr_init(&attrs);
    pthread_attr_setdetachstate(&attrs, PTHREAD_CREATE_JOINABLE);
    int res;
    setThreadParam(param);

    thread_id[0] = 0;
    res = pthread_create(&thread_id[0], &attrs, &pcapLoop, NULL);

    if (res != 0) {
        fprintf(stderr, "Error while creating thread, exiting...\n");
        exit(1);
    }

    thread_id[1] = 0;
    pthread_create(&thread_id[1], &attrs, &printInfo, NULL);

    if (res != 0) {
        fprintf(stderr, "Error while creating thread, exiting...\n");
        exit(1);
    }

    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    pthread_join(thread_id[1], NULL);
    pcap_breakloop(tp->m_descr);
    pthread_join(thread_id[0], NULL);

    std::cout << "Packets captured: " << tp->packets_captured << std::endl;
    std::cout << "Packets processed: " << tp->packets_processed << std::endl;

    return 0;
}

void *pcapLoop(void *)
{
    // Put the device in sniff loop
    pcap_loop(tp->m_descr, -1, processPacket, NULL);
    pcap_close(tp->m_descr);
    return 0;
}

void addFlow(std::map<SrcDst,std::vector<FlowData>> *fd_pf, SrcDst srcdst,
	     FlowData fd) {
    uint64_t samplelen = tp->db2->last - tp->db2->start;
    double r = fd.rate * 1000000.0 / samplelen;

    if (!tp->quiet) {
      printStreamInfo(srcdst);
      printf(" %lu bits/sec\n", (uint64_t)r);
    }

    if (srcdst.m_proto == IPPROTO_TCP || srcdst.m_proto == IPPROTO_UDP ||
	srcdst.m_proto == IPPROTO_ICMP) {
        if (fd_pf->count(srcdst) == 0) {
            std::vector<FlowData> data;
            data.resize(tp->sample_id);
            fd_pf->insert(
		std::pair<SrcDst,std::vector<FlowData>>(srcdst, data));
        }

        if (fd_pf->at(srcdst).size() != tp->sample_id) {
            throw "Sample ID not equal to flow map size";
        }

        fd.rate = r;
        fd_pf->at(srcdst).push_back(fd);
    }
}

void processFD()
{
    if (tp->db2->fm.ecn_rate.size() && !tp->quiet)
	printf("Throughput per stream (ECN queue):\n");
    for (auto& kv: tp->db2->fm.ecn_rate) {
        const SrcDst& srcdst = kv.first;
        FlowData& fd = kv.second;

        addFlow(&tp->fd_pf_ecn, srcdst, fd);
    }

    if (tp->db2->fm.nonecn_rate.size() && !tp->quiet)
	printf("Throughput per stream (non-ECN queue):\n");
    for (auto& kv: tp->db2->fm.nonecn_rate) {
        const SrcDst& srcdst = kv.first;
        FlowData& fd = kv.second;

        addFlow(&tp->fd_pf_nonecn, srcdst, fd);
    }

    // make sure all lists are filled
    for (auto& kv: tp->fd_pf_ecn) {
        if (kv.second.size() != tp->sample_id + 1) {
            kv.second.resize(tp->sample_id + 1);
        }
    }
    for (auto& kv: tp->fd_pf_nonecn) {
        if (kv.second.size() != tp->sample_id + 1) {
            kv.second.resize(tp->sample_id + 1);
        }
    }
}

void wait(uint64_t sleep_ns) {
    struct timespec target;
    clock_gettime(CLOCK_MONOTONIC, &target);

    uint64_t nsec = target.tv_nsec + sleep_ns;
    while (nsec > NSEC_PER_SEC) {
        target.tv_sec++;
        nsec -= NSEC_PER_SEC;
    }

    target.tv_nsec = nsec;

    pthread_mutex_lock(&tp->quit_lock);
    int ret = 0;

    while (ret == 0 && !tp->quit)
        ret = pthread_cond_timedwait(&tp->quit_cond, &tp->quit_lock, &target);

    pthread_mutex_unlock(&tp->quit_lock);
}

void *printInfo(void *)
{
    uint64_t time_ms;

    // per sample
    printf("Output folder: %s\n", tp->m_folder.c_str());

    std::ofstream f_packets_ecn;           openFileW(f_packets_ecn,           tp->m_folder + "/packets_ecn");
    std::ofstream f_packets_nonecn;        openFileW(f_packets_nonecn,        tp->m_folder + "/packets_nonecn");

    std::ofstream f_queue_packets_ecn00;   openFileW(f_queue_packets_ecn00,   tp->m_folder + "/queue_packets_ecn00");
    std::ofstream f_queue_packets_ecn01;   openFileW(f_queue_packets_ecn01,   tp->m_folder + "/queue_packets_ecn01");
    std::ofstream f_queue_packets_ecn10;   openFileW(f_queue_packets_ecn10,   tp->m_folder + "/queue_packets_ecn10");
    std::ofstream f_queue_packets_ecn11;   openFileW(f_queue_packets_ecn11,   tp->m_folder + "/queue_packets_ecn11");
    std::ofstream f_queue_drops_ecn00;     openFileW(f_queue_drops_ecn00,     tp->m_folder + "/queue_drops_ecn00");
    std::ofstream f_queue_drops_ecn01;     openFileW(f_queue_drops_ecn01,     tp->m_folder + "/queue_drops_ecn01");
    std::ofstream f_queue_drops_ecn10;     openFileW(f_queue_drops_ecn10,     tp->m_folder + "/queue_drops_ecn10");
    std::ofstream f_queue_drops_ecn11;     openFileW(f_queue_drops_ecn11,     tp->m_folder + "/queue_drops_ecn11");

    std::ofstream f_rate_ecn;              openFileW(f_rate_ecn,              tp->m_folder + "/rate_ecn");
    std::ofstream f_rate_nonecn;           openFileW(f_rate_nonecn,           tp->m_folder + "/rate_nonecn");
    std::ofstream f_drops_ecn;             openFileW(f_drops_ecn,             tp->m_folder + "/drops_ecn");
    std::ofstream f_drops_nonecn;          openFileW(f_drops_nonecn,          tp->m_folder + "/drops_nonecn");
    std::ofstream f_marks_ecn;             openFileW(f_marks_ecn,             tp->m_folder + "/marks_ecn");
    std::ofstream f_rate;                  openFileW(f_rate,                  tp->m_folder + "/rate");

    // first column in header contains the number of columns following
    f_queue_packets_ecn00 << QS_LIMIT;
    f_queue_packets_ecn01 << QS_LIMIT;
    f_queue_packets_ecn10 << QS_LIMIT;
    f_queue_packets_ecn11 << QS_LIMIT;
    f_queue_drops_ecn00 << QS_LIMIT;
    f_queue_drops_ecn01 << QS_LIMIT;
    f_queue_drops_ecn10 << QS_LIMIT;
    f_queue_drops_ecn11 << QS_LIMIT;

    // header row contains the queue delay this column represents
    // e.g. a cell value multiplied by this header cell yields queue delay in us
    for (int i = 0; i < QS_LIMIT; ++i) {
        f_queue_packets_ecn00 << " " << tp->qdelay_decode_table[i];
        f_queue_packets_ecn01 << " " << tp->qdelay_decode_table[i];
        f_queue_packets_ecn10 << " " << tp->qdelay_decode_table[i];
        f_queue_packets_ecn11 << " " << tp->qdelay_decode_table[i];
        f_queue_drops_ecn00 << " " << tp->qdelay_decode_table[i];
        f_queue_drops_ecn01 << " " << tp->qdelay_decode_table[i];
        f_queue_drops_ecn10 << " " << tp->qdelay_decode_table[i];
        f_queue_drops_ecn11 << " " << tp->qdelay_decode_table[i];
    }
    f_queue_packets_ecn00 << std::endl;
    f_queue_packets_ecn01 << std::endl;
    f_queue_packets_ecn10 << std::endl;
    f_queue_packets_ecn11 << std::endl;
    f_queue_drops_ecn00 << std::endl;
    f_queue_drops_ecn01 << std::endl;
    f_queue_drops_ecn10 << std::endl;
    f_queue_drops_ecn11 << std::endl;

    // first run
    // to get accurate results we swap the database and initialize timers here
    // (this way we don't time wrong and gets packets outside our time area)
    tp->db2->init();
    tp->swapDB();
    tp->start = tp->db1->start;

    wait(tp->m_sinterval * NSEC_PER_MS);

    uint64_t elapsed, next, sleeptime;

    while (1) {
        tp->swapDB();

        // time since we started processing
        time_ms = (tp->db2->last - tp->start) / 1000;
        tp->sample_times.push_back(time_ms);

        printf("\n--- BEGIN SAMPLE # %d", (int) tp->sample_id + 1);
        if (tp->m_nrs != 0) {
            printf(" of %d", tp->m_nrs);
        }
        printf(" -- total run time %d ms ---\n", (int) time_ms);

        printf(" delay [ms]    ");
        printf(" ECN 00: ");
        printf(" ECN 01: ");
        printf(" ECN 10: ");
        printf(" ECN 11: \n");

        f_queue_packets_ecn00 << time_ms;
        f_queue_packets_ecn01 << time_ms;
        f_queue_packets_ecn10 << time_ms;
        f_queue_packets_ecn11 << time_ms;
        f_queue_drops_ecn00 << time_ms;
        f_queue_drops_ecn01 << time_ms;
        f_queue_drops_ecn10 << time_ms;
        f_queue_drops_ecn11 << time_ms;

        for (int i = 0; i < QS_LIMIT; ++i) {
            if (tp->db2->qs.ecn00[i] > 0 || tp->db2->qs.ecn01[i] > 0 ||
		tp->db2->qs.ecn10[i] > 0 || tp->db2->qs.ecn11[i] > 0) {
                // TODO: can we make it less verbose? e.g. group by some intervals?
                printf("%9.3f:  %8d %8d %8d %8d\n",
                    (double) tp->qdelay_decode_table[i] / 1000,
                    tp->db2->qs.ecn00[i],
                    tp->db2->qs.ecn01[i],
                    tp->db2->qs.ecn10[i],
                    tp->db2->qs.ecn11[i]
                );
            }

            f_queue_packets_ecn00 << " " << tp->db2->qs.ecn00[i];
            f_queue_packets_ecn01 << " " << tp->db2->qs.ecn01[i];
            f_queue_packets_ecn10 << " " << tp->db2->qs.ecn10[i];
            f_queue_packets_ecn11 << " " << tp->db2->qs.ecn11[i];
            f_queue_drops_ecn00 << " " << tp->db2->d_qs.ecn00[i];
            f_queue_drops_ecn01 << " " << tp->db2->d_qs.ecn01[i];
            f_queue_drops_ecn10 << " " << tp->db2->d_qs.ecn10[i];
            f_queue_drops_ecn11 << " " << tp->db2->d_qs.ecn11[i];
        }

        f_queue_packets_ecn00 << std::endl;
        f_queue_packets_ecn01 << std::endl;
        f_queue_packets_ecn10 << std::endl;
        f_queue_packets_ecn11 << std::endl;
        f_queue_drops_ecn00 << std::endl;
        f_queue_drops_ecn01 << std::endl;
        f_queue_drops_ecn10 << std::endl;
        f_queue_drops_ecn11 << std::endl;

        f_rate_ecn     << tp->sample_id << " " << time_ms;
        f_rate_nonecn  << tp->sample_id << " " << time_ms;
        f_drops_ecn    << tp->sample_id << " " << time_ms;
        f_drops_nonecn << tp->sample_id << " " << time_ms;
        f_marks_ecn    << tp->sample_id << " " << time_ms;
        f_rate         << tp->sample_id << " " << time_ms;

        processFD();

        double rate_ecn = 0;
        double rate_nonecn = 0;
        uint64_t drops_ecn = 0;
        uint64_t drops_nonecn = 0;
        uint64_t marks_ecn = 0;

        for (auto const& val: tp->fd_pf_ecn) {
            rate_ecn += val.second.at(tp->sample_id).rate;
            drops_ecn += val.second.at(tp->sample_id).drops;
            marks_ecn += val.second.at(tp->sample_id).marks;
        }

        f_rate_ecn << " " << rate_ecn;
        f_drops_ecn << " " << drops_ecn;
        f_marks_ecn << " " << marks_ecn;

        for (auto const& val: tp->fd_pf_nonecn) {
            rate_nonecn += val.second.at(tp->sample_id).rate;
            drops_nonecn += val.second.at(tp->sample_id).drops;
        }

        f_rate_nonecn << " " << rate_nonecn;
        f_drops_nonecn << " " << drops_nonecn;

        f_rate << " " << (rate_ecn + rate_nonecn);

        f_rate << std::endl;
        f_rate_ecn << std::endl;
        f_rate_nonecn << std::endl;
        f_drops_ecn << std::endl;
        f_drops_nonecn << std::endl;
        f_marks_ecn << std::endl;

        f_packets_ecn << tp->db2->tot_packets_ecn << std::endl;
        f_packets_nonecn << tp->db2->tot_packets_nonecn << std::endl;

        tp->packets_processed += tp->db2->tot_packets_nonecn +
		tp->db2->tot_packets_ecn;

        printf("Total throughput: %lu bits/sec\n",
	       (uint64_t)(rate_nonecn + rate_ecn));

        printf("--- END SAMPLE # %d", (int) tp->sample_id + 1);
        if (tp->m_nrs != 0) {
            printf(" of %d", tp->m_nrs);
        }
        printf(" -- \n\n");

        if (tp->m_nrs != 0 && tp->sample_id >= (tp->m_nrs - 1)) {
            printf("Obtained given number of samples (%d)\n", tp->m_nrs);
            break;
        }

        tp->db2->init(); // init outside the critical area to save time

        elapsed = getStamp() - tp->start;
        next = ((uint64_t) tp->sample_id + 2) *
		tp->m_sinterval * 1000; // convert ms to us

        int process_time = getStamp() - tp->db2->last;
        if (elapsed < next) {
            uint64_t sleeptime = next - elapsed;
            printf("Processed data in approx. %d us - sleeping for %d us\n",
		   (int) process_time, (int) sleeptime);
            wait(sleeptime * NSEC_PER_US);
        }

        if (tp->quit) {
            break;
        }

        tp->sample_id++;
    }

    f_queue_packets_ecn00.close();
    f_queue_packets_ecn01.close();
    f_queue_packets_ecn10.close();
    f_queue_packets_ecn11.close();

    f_queue_drops_ecn00.close();
    f_queue_drops_ecn01.close();
    f_queue_drops_ecn10.close();
    f_queue_drops_ecn11.close();

    f_packets_ecn.close();
    f_packets_nonecn.close();

    f_rate_ecn.close();
    f_rate_nonecn.close();
    f_drops_ecn.close();
    f_drops_nonecn.close();
    f_marks_ecn.close();
    f_rate.close();

    // write per flow stats
    // (we wait till here because we don't know how many
    //  flows there are before the test is finished)
    std::ofstream f_flows_rate_ecn;     openFileW(f_flows_rate_ecn,      tp->m_folder + "/flows_rate_ecn");
    std::ofstream f_flows_rate_nonecn;  openFileW(f_flows_rate_nonecn,   tp->m_folder + "/flows_rate_nonecn");
    std::ofstream f_flows_drops_ecn;    openFileW(f_flows_drops_ecn,     tp->m_folder + "/flows_drops_ecn");
    std::ofstream f_flows_drops_nonecn; openFileW(f_flows_drops_nonecn,  tp->m_folder + "/flows_drops_nonecn");
    std::ofstream f_flows_marks_ecn;    openFileW(f_flows_marks_ecn,     tp->m_folder + "/flows_marks_ecn");

    // note: drop and mark numbers per flow don't really tell us much, as
    //       the numbers include whichever packet was handled before this
    //       in the same queue
    //       e.g. a drop might be for another flow

    for (int i = 0; i < tp->sample_times.size(); i++) {
        f_flows_rate_ecn << i << " " << tp->sample_times[i];
        f_flows_drops_ecn << i << " " << tp->sample_times[i];
        f_flows_marks_ecn << i << " " << tp->sample_times[i];

        f_flows_rate_nonecn << i << " " << tp->sample_times[i];
        f_flows_drops_nonecn << i << " " << tp->sample_times[i];

        for (auto const& kv: tp->fd_pf_ecn) {
            f_flows_rate_ecn << " " << kv.second.at(i).rate;
            f_flows_drops_ecn << " " << kv.second.at(i).drops;
            f_flows_marks_ecn << " " << kv.second.at(i).marks;
        }

        for (auto const& kv: tp->fd_pf_nonecn) {
            f_flows_rate_nonecn << " " << kv.second.at(i).rate;
            f_flows_drops_nonecn << " " << kv.second.at(i).drops;
        }

        f_flows_rate_ecn << std::endl;
        f_flows_drops_ecn << std::endl;
        f_flows_marks_ecn << std::endl;

        f_flows_rate_nonecn << std::endl;
        f_flows_drops_nonecn << std::endl;
    }

    f_flows_rate_ecn.close();
    f_flows_rate_nonecn.close();
    f_flows_drops_ecn.close();
    f_flows_drops_nonecn.close();
    f_flows_marks_ecn.close();

    // save flow details
    std::ofstream f_flows_ecn;    openFileW(f_flows_ecn,    tp->m_folder + "/flows_ecn");
    std::ofstream f_flows_nonecn; openFileW(f_flows_nonecn, tp->m_folder + "/flows_nonecn");

    for (auto const& kv: tp->fd_pf_ecn) {
        f_flows_ecn << getProtoRepr(kv.first.m_proto) << " "
		<< IPtoString(kv.first.m_srcip) << " "
		<< kv.first.m_srcport << " "
		<< IPtoString(kv.first.m_dstip) << " "
		<< kv.first.m_dstport << std::endl;
    }

    for (auto const& kv: tp->fd_pf_nonecn) {
        f_flows_nonecn << getProtoRepr(kv.first.m_proto) << " "
		<< IPtoString(kv.first.m_srcip) << " "
		<< kv.first.m_srcport << " "
		<< IPtoString(kv.first.m_dstip) << " "
		<< kv.first.m_dstport << std::endl;
    }

    f_flows_ecn.close();
    f_flows_nonecn.close();

    return 0;
}


