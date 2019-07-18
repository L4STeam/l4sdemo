#include "linkaqm.h"
#include "resources.h"
#include "script_runner.h"

#include <QColor>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <qwt_plot.h>
#include <QStringList>
#include <QFont>
#include <iostream>
#include <fstream>
#include <sstream>

Linkaqm::Linkaqm(QWidget *parent)
    : QGroupBox(parent)
    , utilValue(0.0)
    , m_mark_perc_ll(0.0)
    , m_drop_perc_ll(0.0)
    , m_mark_perc_c(0.0)
    , m_drop_perc_c(0.0)
{

    QFont axisFont("Times New Roman", 10);
    QFont titleFont("Helvetica", 11);
    titleFont.setBold(true);
    QwtText title;
    title.setFont(titleFont);
    readAQMList();
    readLinkCapList();
    readBrttList();

    this->setMaximumHeight(600);
    this->setMinimumWidth(600);
    QColor cblue(Qt::blue);
    QColor corange(255, 157, 0);
    QFont displayFont("Times New Roman", 18);
    displayFont.setBold(true);

    QStringList markdropList;
    markdropList.append("auto");
    markdropList.append("0 - 3");
    markdropList.append("0 - 10");
    markdropList.append("0 - 20");
    markdropList.append("0 - 40");
    markdropList.append("0 - 60");
    markdropList.append("0 - 80");
    markdropList.append("0 - 100");

    combollmark = new QComboBox(this);
    combollmark->insertItems(0, markdropList);
    combolldrop = new QComboBox(this);
    combolldrop->insertItems(0, markdropList);
    combocmark = new QComboBox(this);
    combocmark->insertItems(0, markdropList);
    combocdrop = new QComboBox(this);
    combocdrop->insertItems(0, markdropList);
    aqmselect = new QComboBox(this);
    aqmselect->insertItems(0, aqmList);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    QGridLayout *dataDisplayLayout = new QGridLayout;

    QHBoxLayout *aqm = new QHBoxLayout;
    QLabel *titleAQM = new QLabel("AQM: ", this);
    aqm->addWidget(titleAQM);
    aqm->addWidget(aqmselect);

    QWidget *emptyWidget60 = new QWidget();
    emptyWidget60->setFixedHeight(60);
    QWidget *emptyWidget30 = new QWidget();
    emptyWidget30->setFixedHeight(30);

    QVBoxLayout *llmarkdrop = new QVBoxLayout;
    QHBoxLayout *markvaluell = new QHBoxLayout;
    QHBoxLayout *markyscalell = new QHBoxLayout;
    QHBoxLayout *dropvaluell = new QHBoxLayout;
    QHBoxLayout *dropyscalell = new QHBoxLayout;

    QLabel *titleMarkLL = new QLabel("L4S Mark [%]:", this);
    displayMarkLL = new QLabel("0", this);
    displayMarkLL->setFont(displayFont);
    displayMarkLL->setFixedWidth(40);
    markvaluell->addWidget(titleMarkLL);
    markvaluell->addWidget(displayMarkLL);
    markvaluell->setAlignment(displayMarkLL, Qt::AlignRight);
    llmarkdrop->setContentsMargins(0,0,0,0);
    llmarkdrop->addLayout(markvaluell);
    QLabel *titleYScaleMarkLL = new QLabel("Y-scale:", this);
    markyscalell->addWidget(titleYScaleMarkLL);
    markyscalell->addWidget(combollmark);
    llmarkdrop->addLayout(markyscalell);
    llmarkdrop->addWidget(emptyWidget60);

    QLabel *titleDropLL = new QLabel("L4S Drop [%]:", this);
    displayDropLL = new QLabel("0", this);
    displayDropLL->setFont(displayFont);
    displayDropLL->setFixedWidth(40);
    dropvaluell->addWidget(titleDropLL);
    dropvaluell->addWidget(displayDropLL);
    markvaluell->setAlignment(displayDropLL, Qt::AlignRight);

    llmarkdrop->addLayout(dropvaluell);
    QLabel *titleYScaleDropLL = new QLabel("Y-scale:", this);
    dropyscalell->addWidget(titleYScaleDropLL);
    dropyscalell->addWidget(combolldrop);
    llmarkdrop->addLayout(dropyscalell);
    llmarkdrop->addWidget(emptyWidget60);

    QVBoxLayout *llhistory = new QVBoxLayout;
    plotMarkHistoryLL = new HistoryPlot(this, cblue);
    plotDropHistoryLL = new HistoryPlot(this, cblue);
    llhistory->addWidget(plotMarkHistoryLL);
    llhistory->addWidget(plotDropHistoryLL);

    QHBoxLayout *llqueue = new QHBoxLayout;
    plotLLQueue = new TrafficPlot(this, cblue);
    title.setText("Client A delay [ms]");
    plotLLQueue->setAxisTitle(2, title);
    llqueue->addWidget(plotLLQueue);

    QVBoxLayout *cmarkdrop = new QVBoxLayout;
    QHBoxLayout *markvaluec = new QHBoxLayout;
    QHBoxLayout *markyscalec = new QHBoxLayout;
    QHBoxLayout *dropvaluec = new QHBoxLayout;
    QHBoxLayout *dropyscalec = new QHBoxLayout;

    QLabel *titleMarkC = new QLabel("Classic Mark [%]:", this);
    displayMarkC = new QLabel("0", this);
    displayMarkC->setFont(displayFont);
    displayMarkC->setFixedWidth(40);
    markvaluec->addWidget(titleMarkC);
    markvaluec->addWidget(displayMarkC);
    markvaluec->setAlignment(displayMarkC, Qt::AlignRight);

    cmarkdrop->setContentsMargins(0,0,0,0);
    cmarkdrop->addLayout(markvaluec);
    QLabel *titleYScaleMarkC = new QLabel("Y-scale:", this);
    markyscalec->addWidget(titleYScaleMarkC);
    markyscalec->addWidget(combocmark);
    cmarkdrop->addLayout(markyscalec);
    cmarkdrop->addWidget(emptyWidget60);

    QLabel *titleDropC = new QLabel("Classic Drop [%]:", this);
    displayDropC = new QLabel("0", this);
    displayDropC->setFont(displayFont);
    displayDropC->setFixedWidth(40);
    dropvaluec->addWidget(titleDropC);
    dropvaluec->addWidget(displayDropC);
    markvaluec->setAlignment(displayDropC, Qt::AlignRight);

    cmarkdrop->addLayout(dropvaluec);
    QLabel *titleYScaleDropC = new QLabel("Y-scale:", this);
    dropyscalec->addWidget(titleYScaleDropC);
    dropyscalec->addWidget(combocdrop);
    cmarkdrop->addLayout(dropyscalec);
    cmarkdrop->addWidget(emptyWidget60);

    QHBoxLayout *cqueue = new QHBoxLayout;
    plotCQueue = new TrafficPlot(this, corange);
    title.setText("Client B delay [ms]");
    plotCQueue->setAxisTitle(2, title);
    cqueue->addWidget(plotCQueue);

    QVBoxLayout *chistory = new QVBoxLayout;
    plotMarkHistoryC = new HistoryPlot(this, corange);
    plotDropHistoryC = new HistoryPlot(this, corange);
    chistory->addWidget(plotMarkHistoryC);
    chistory->addWidget(plotDropHistoryC);

    QVBoxLayout *utilhistory = new QVBoxLayout;
    QHBoxLayout *linkCap = new QHBoxLayout;
    QLabel *titleLink = new QLabel("Link [Mbps]:", this);
    QLabel *titleBrtt = new QLabel("Base RTT [ms]:", this);
    linkselect = new QComboBox(this);
    linkselect->setMinimumWidth(50);
    linkselect->insertItems(0, linkCapList);
    brttselect = new QComboBox(this);
    brttselect->insertItems(0, brttList);
    brttselect->setMinimumWidth(50);
    linkCap->addWidget(titleLink);
    linkCap->addWidget(linkselect);
    QHBoxLayout *baseRTT = new QHBoxLayout;
    baseRTT->addWidget(titleBrtt);
    baseRTT->addWidget(brttselect);
    QColor c(Qt::green);
    plotUtilHistory = new HistoryPlot(this, c);
    plotUtilHistory->updateAxisScale(100, 50);
    plotUtilHistory->setAxisFont(0, axisFont);
    plotUtilHistory->setAxisFont(2, axisFont);

    utilhistory->addWidget(plotUtilHistory);

    QHBoxLayout *utilValueLayout = new QHBoxLayout;
    QLabel *titleUtil = new QLabel("Utilisation [%]: ", this);
    displayUtil = new QLabel("0", this);
    displayUtil->setFont(displayFont);
    utilValueLayout->addWidget(titleUtil);
    utilValueLayout->addWidget(displayUtil);
    QHBoxLayout *displayUtilLayout = new QHBoxLayout;
    displayUtilLayout->addLayout(utilValueLayout);
    QVBoxLayout *linkUtilBoxLayout = new QVBoxLayout;
    linkUtilBoxLayout->addLayout(displayUtilLayout);
    linkUtilBoxLayout->addWidget(emptyWidget30);

    QVBoxLayout *linkRttSelectLayout = new QVBoxLayout;
    linkRttSelectLayout->addLayout(linkCap);
    linkRttSelectLayout->addLayout(baseRTT);
    linkRttSelectLayout->setAlignment(linkCap, Qt::AlignRight);
    linkRttSelectLayout->setAlignment(baseRTT, Qt::AlignRight);

    QHBoxLayout *avgl4s = new QHBoxLayout;
    QHBoxLayout *avgc = new QHBoxLayout;
    QLabel *titleAvgL4S = new QLabel("Client A delay[ms] avg:", this);
    QLabel *titleAvgC = new QLabel("Client B delay[ms] avg:", this);
    displayAvgQSLL = new QLabel("0", this);
    displayAvgQSLL->setFixedWidth(40);
    displayAvgQSC = new QLabel("0", this);
    displayAvgQSC->setFixedWidth(40);
    displayAvgQSLL->setFont(displayFont);
    displayAvgQSC->setFont(displayFont);

    QLabel *titleP99L4S = new QLabel("P99:", this);
    QLabel *titleP99C = new QLabel("P99:", this);
    displayP99QSLL = new QLabel("0", this);
    displayP99QSLL->setFixedWidth(40);
    displayP99QSC = new QLabel("0", this);
    displayP99QSC->setFixedWidth(40);
    displayP99QSLL->setFont(displayFont);
    displayP99QSC->setFont(displayFont);

    avgl4s->addWidget(titleAvgL4S);
    avgl4s->addWidget(displayAvgQSLL);
    avgl4s->addWidget(titleP99L4S);
    avgl4s->addWidget(displayP99QSLL);
    avgl4s->setAlignment(titleP99L4S, Qt::AlignRight);
    avgc->addWidget(titleAvgC);
    avgc->addWidget(displayAvgQSC);
    avgc->addWidget(titleP99C);
    avgc->addWidget(displayP99QSC);
    avgc->setAlignment(titleP99C, Qt::AlignRight);

    QVBoxLayout *middleRowLayout = new QVBoxLayout;
    middleRowLayout->addLayout(avgl4s);
    middleRowLayout->addLayout(linkRttSelectLayout);
    middleRowLayout->addLayout(avgc);

    dataDisplayLayout->addLayout(aqm, 0,3,1,1);
    dataDisplayLayout->addLayout(llmarkdrop, 1,2,1,1);
    dataDisplayLayout->addLayout(llhistory, 1,1,1,1);
    dataDisplayLayout->addLayout(llqueue, 1,3,1,1);
    dataDisplayLayout->addLayout(linkUtilBoxLayout, 2,2,1,1);
    dataDisplayLayout->addLayout(middleRowLayout, 2,3,1,1);
    dataDisplayLayout->addLayout(utilhistory, 2,1,1,1);
    dataDisplayLayout->addLayout(cmarkdrop, 3,2,1,1);
    dataDisplayLayout->addLayout(chistory, 3,1,1,1);
    dataDisplayLayout->addLayout(cqueue, 3,3,1,1);
    mainLayout->addLayout(dataDisplayLayout);

    connect(this, SIGNAL(scheduleRepaint()), this, SLOT(commitData()));connect(combollmark, SIGNAL(currentIndexChanged(int)), this, SLOT(updateComboLLMark(int)));
    connect(combolldrop, SIGNAL(currentIndexChanged(int)), this, SLOT(updateComboLLDrop(int)));
    connect(combocmark, SIGNAL(currentIndexChanged(int)), this, SLOT(updateComboCMark(int)));
    connect(combocdrop, SIGNAL(currentIndexChanged(int)), this, SLOT(updateComboCDrop(int)));
    connect(aqmselect, SIGNAL(currentIndexChanged(int)), this, SLOT(updateAQM(int)));
    connect(linkselect, SIGNAL(currentIndexChanged(int)), this, SLOT(updateLink(int)));
    connect(brttselect, SIGNAL(currentIndexChanged(int)), this, SLOT(updateBrtt(int)));

     if (aqmList.size() > 0)
        updateAQM(0);

}

void Linkaqm::updateData(std::vector<double> yData_ll, std::vector<double> yData_c, double mark_perc_ll, double drop_perc_ll, double mark_perc_c,
                         double drop_perc_c, int util, double avgqs_ll, double avgqs_c, double p99qs_ll, double p99qs_c)
{
    QMutexLocker m(&dataMutex);
    m_yData_ll = yData_ll;
    m_yData_c = yData_c;
    m_mark_perc_ll = mark_perc_ll;
    m_drop_perc_ll = drop_perc_ll;
    m_mark_perc_c = mark_perc_c;
    m_drop_perc_c = drop_perc_c;
    m_avgqs_ll = avgqs_ll;
    m_avgqs_c = avgqs_c;
    m_p99qs_ll = p99qs_ll;
    m_p99qs_c = p99qs_c;
    utilValue = util;

    scheduleRepaint();
}

void Linkaqm::commitData()
{
    QMutexLocker m(&dataMutex);

    displayUtil->setText(QString().setNum(utilValue));
    displayAvgQSLL->setNum(m_avgqs_ll);
    displayAvgQSC->setNum(m_avgqs_c);
    displayP99QSLL->setNum(m_p99qs_ll);
    displayP99QSC->setNum(m_p99qs_c);
    plotUtilHistory->updateCurve(utilValue);

    plotLLQueue->updateCurve(m_yData_ll);
    plotCQueue->updateCurve(m_yData_c);
    displayMarkLL->setNum(m_mark_perc_ll);
    plotMarkHistoryLL->updateCurve(m_mark_perc_ll);
    displayDropLL->setNum(m_drop_perc_ll);
    plotDropHistoryLL->updateCurve(m_drop_perc_ll);
    displayMarkC->setNum(m_mark_perc_c);
    plotMarkHistoryC->updateCurve(m_mark_perc_c);
    displayDropC->setNum(m_drop_perc_c);
    plotDropHistoryC->updateCurve(m_drop_perc_c);
}

void Linkaqm::updateComboLLMark(int num)
{
    updatePlotScale(plotMarkHistoryLL, num);
}

void Linkaqm::updateComboLLDrop(int num)
{
   updatePlotScale(plotDropHistoryLL, num);
}

void Linkaqm::updateComboCMark(int num)
{
    updatePlotScale(plotMarkHistoryC, num);
}

void Linkaqm::updateComboCDrop(int num)
{
    updatePlotScale(plotDropHistoryC, num);
}

void Linkaqm::update_link_properties()
{
    int linkindex = linkselect->currentIndex();
    int aqmindex = aqmselect->currentIndex();
    int brttindex = brttselect->currentIndex();
    std::stringstream command;
    command << res_path("/sh/set_aqm_link.sh") << " "
	    << aqmIDList.at(aqmindex) << " "
	    << linkCapValues.at(linkindex) << " "
	    << brttValues.at(brttindex);
    _RUN_SCRIPT(command.str());
}

void Linkaqm::updateAQM(int num)
{
    (void)num;
    update_link_properties();
}

void Linkaqm::updateLink(int num)
{
    update_link_properties();
    linkChanged(linkCapValues.at(num));
}

void Linkaqm::updateBrtt(int num)
{
    update_link_properties();
    brttChanged(brttValues.at(num));
}

void Linkaqm::updatePlotScale(HistoryPlot *plot, int num)
{
    if (num == 0)
         plot->updateAxisScale(0, 0);
    else if (num == 1)
         plot->updateAxisScale(3, 1);
    else if (num == 2)
         plot->updateAxisScale(10, 5);
    else if (num == 3)
         plot->updateAxisScale(20, 10);
    else if (num == 4)
         plot->updateAxisScale(40, 20);
    else if (num == 5)
         plot->updateAxisScale(60, 30);
    else if (num == 6)
         plot->updateAxisScale(80, 40);
    else if (num == 7)
         plot->updateAxisScale(100, 50);
}

void Linkaqm::readAQMList(){
    std::ifstream infile(res_path( "/config/aqm_list"));
    if (infile.is_open()) {
        std::string aqm_name;
        std::string aqm_command;
        while (!infile.eof()){
            std::stringstream ss_aqm_name;
            infile >> aqm_name;
            ss_aqm_name << aqm_name;
            infile >> aqm_name;
            while (!infile.eof() && aqm_name != ":") {
                ss_aqm_name << " " << aqm_name;
                infile >> aqm_name;
            }
            if (!infile.eof()) {
                infile >> aqm_command;
                aqmList.append(ss_aqm_name.str().c_str());
                aqmIDList.push_back(aqm_command);
            }
        }
        infile.close();
    } else
        std::cerr << "ERROR: 'aqm_list' file is missing or corrupted." << std::endl;
}

void Linkaqm::readLinkCapList(){
    std::ifstream infile(res_path("/config/link_cap_list"));
    if (infile.is_open()) {
        std::string link_cap;
        int linkCapValue;
        while (!infile.eof()) {
            infile >> link_cap;
            if (atoi(link_cap.c_str()) != linkCapValue) {
                linkCapValue = atoi(link_cap.c_str());
                linkCapList.append(link_cap.c_str());
                linkCapValues.push_back(linkCapValue);
            }
        }
        infile.close();
    } else
        std::cerr << "ERROR: 'link_cap_list' file is missing or corrupted." << std::endl;
}

void Linkaqm::readBrttList()
{
    std::ifstream infile(res_path( "/config/brtt_list"));
    if (infile.is_open()) {
        std::string rtt;
        int rttValue;
        while (!infile.eof()) {
            infile >> rtt;
            if (atoi(rtt.c_str()) != rttValue) {
                rttValue = atoi(rtt.c_str());
                brttList.append(rtt.c_str());
                brttValues.push_back(rttValue);
            }
        }
        infile.close();
    } else
        std::cerr << "ERROR: 'brtt_list' file is missing or corrupted." << std::endl;
}

