#ifndef ANALYSIS_H
#define ANALYSIS_H

#include <string>
#include <map>
#include <pcap.h> /* if this gives you an error try pcap/pcap.h */
#include <pthread.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <string.h>
#include <vector>

#define QS_LIMIT 2048
#define PDF_UPPERLIM 500
#define NSEC_PER_SEC 1000000000UL
#define NSEC_PER_MS 1000000UL
#define US_PER_S 1000000UL
#define NSEC_PER_US 1000UL

struct SrcDst {
public:
    SrcDst(uint8_t proto, in_addr_t srcip, uint16_t srcport,
	   in_addr_t dstip, uint16_t dstport)
      : m_proto(proto), m_srcip(srcip), m_dstip(dstip),
	m_srcport(srcport), m_dstport(dstport) {}

    uint8_t m_proto;
    in_addr_t m_srcip, m_dstip;
    uint16_t m_srcport, m_dstport;

    bool operator<(const SrcDst& rhs) const {
        if (m_proto != rhs.m_proto)
            return m_proto < rhs.m_proto;
        if (m_srcip != rhs.m_srcip)
            return m_srcip < rhs.m_srcip;
        if (m_dstip != rhs.m_dstip)
            return m_dstip < rhs.m_dstip;
        if (m_srcport != rhs.m_srcport)
            return m_srcport < rhs.m_srcport;
        return m_dstport < rhs.m_dstport;
    }

    bool operator==(const SrcDst &rhs) const {
        return m_srcip == rhs.m_srcip &&
               m_dstip == rhs.m_dstip &&
               m_srcport == rhs.m_srcport &&
               m_dstport == rhs.m_dstport;
    }
};

struct FlowData {
public:
    FlowData(uint64_t r, uint32_t d, uint32_t m) {
        rate = r;
        drops = d;
        marks = m;
    }

    FlowData() : rate(0), drops(0), marks(0) {}

    void update(uint32_t r, uint32_t d, uint32_t m) {
        rate += r;
        drops += d;
        marks += m;
    }

    void clear() {
        rate = 0;
        drops = 0;
        marks = 0;
    }

    uint64_t rate;
    uint32_t drops;
    uint32_t marks;

};

struct FlowMap {
public:
    std::map<SrcDst,FlowData> ecn_rate;
    std::map<SrcDst,FlowData> nonecn_rate;
    void init(){
        ecn_rate.clear();
        nonecn_rate.clear();
        /*for (std::map<SrcDst,FlowData>::iterator it = ecn_rate.begin(); it != ecn_rate.end(); ++it)
            it->second.clear();
        for (std::map<SrcDst,FlowData>::iterator it = nonecn_rate.begin(); it != nonecn_rate.end(); ++it)
            it->second.clear();*/
    }
};

struct QueueSize {
public:
    uint32_t ecn00[QS_LIMIT];
    uint32_t ecn01[QS_LIMIT];
    uint32_t ecn10[QS_LIMIT];
    uint32_t ecn11[QS_LIMIT];

    void init(){
        bzero(ecn00,QS_LIMIT*sizeof(uint32_t));
        bzero(ecn01,QS_LIMIT*sizeof(uint32_t));
        bzero(ecn10,QS_LIMIT*sizeof(uint32_t));
        bzero(ecn11,QS_LIMIT*sizeof(uint32_t));
    }
};

struct DataBlock {
public:
    struct QueueSize qs;
    struct QueueSize d_qs; // total number of drops for each queue size
    struct FlowMap fm;
    uint64_t start; // time in us
    uint64_t last;  // time in us
    uint64_t tot_packets_ecn;
    uint64_t tot_packets_nonecn;

    void init(){
        qs.init();
        d_qs.init();
        fm.init();
        tot_packets_ecn = 0;
        tot_packets_nonecn = 0;
    }
};

struct ThreadParam {
public:
    // table of qdelay values (no need to decode all the time..)
    int qdelay_decode_table[QS_LIMIT];

    uint64_t packets_captured;
    uint64_t packets_processed;
    uint64_t start;
    pthread_mutex_t m_mutex;
    DataBlock *db1; // used by ProcessPacket
    DataBlock *db2; // used by printInfo
    pcap_t* m_descr;
    uint32_t m_sinterval;
    std::string m_folder;
    bool ipclass;
    uint32_t m_nrs;
    std::map<SrcDst,std::vector<FlowData>> fd_pf_ecn;
    std::map<SrcDst,std::vector<FlowData>> fd_pf_nonecn;
    ThreadParam(uint32_t sinterval, std::string folder, bool ipc, uint32_t nrs);
    void swapDB();
    volatile bool quit;
    pthread_cond_t quit_cond;
    pthread_mutex_t quit_lock;
    int sample_id;
    std::vector<uint64_t> sample_times;
};

uint64_t getStamp();

void *pcapLoop(void *);
int setup_pcap(ThreadParam *param, const char *dev, std::string &pcapfilter);
int start_analysis(ThreadParam *param);
void processFD();
void wait(uint64_t sleep_ns);
void setThreadParam(ThreadParam *param);

#endif // ANALYSIS_H
