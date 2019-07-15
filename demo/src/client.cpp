#include "client.h"
#include "resources.h"
#include "script_runner.h"
#include <qwt_scale_draw.h>
#include <qwt_scale_widget.h>
#include <qwt_plot_marker.h>
#include <qwt_column_symbol.h>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QMutexLocker>
#include <QSlider>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <sstream>
#include <fstream>
#include <qwt_color_map.h>

class NoScaleDraw : public QwtScaleDraw {
protected:
    virtual void 	drawTick (QPainter *, double val, double len) const {}
    virtual void 	drawBackbone (QPainter *) const {}
    virtual void 	drawLabel (QPainter *, double val) const {}
};

class UnrelatedNumberScaleDraw : public QwtScaleDraw {
public:
    UnrelatedNumberScaleDraw()
        : unrelatedNumber(0.0)
    {}
    void setNumber(double num) { unrelatedNumber = num;}
    void setUnit(QString u) { unit = u; }
protected:
    virtual void 	drawTick (QPainter *, double val, double len) const {}
    virtual void 	drawBackbone (QPainter *) const {}
    virtual void 	drawLabel (QPainter *painter, double value) const
    {
        if (value < 99 && value > 101) return;
        QwtText lbl(QString().setNum(unrelatedNumber) + unit); // = tickLabel( painter->font(), unrelatedNumber );
        lbl.setRenderFlags( 0 );
        lbl.setLayoutAttribute(QwtText::MinimumLayout);
        if ( lbl.isEmpty() )
            return;

        QPointF pos = labelPosition( 100 );
        QSizeF labelSize = lbl.textSize( painter->font() );
        const QTransform transform = labelTransformation( pos, labelSize );

        painter->save();
        painter->setWorldTransform( transform, true );

        lbl.draw ( painter, QRect( QPoint( 0, 0 ), labelSize.toSize() ) );
        painter->restore();
    }
private:
    double unrelatedNumber;
    QString unit;
};

Client::Client(QWidget *parent, const char* download_path, const char* killall_path, const char* killdownload_path,
               const char* wb_path, const char* rtt_path, const char* cc_path,
               const char* al_path, const char* cbr_path, const QColor& color)
    : QGroupBox(parent)
    , linkcap(40)
    , displayNumDownloads(0)
    , noScaleDraw(0)
    , unrelatedNumberScaleDraw(0)
    , noScaleDrawAL(0)
    , rate(0.0)
    , nrFlows(0)
    , complHS(1)
    , ssh_killall(killall_path)
    , ssh_killdownload(killdownload_path)
    , ssh_download(download_path)
    , ssh_wb(wb_path)
    , ssh_rtt(rtt_path)
    , ssh_al(al_path)
    , ssh_cbr(cbr_path)
    , ssh_cc(cc_path)


{

    QFont axisFont("Times New Roman", 10);
    QFont titleFont("Helvetica", 12);
    QFont smallTitleFont("Helvetica", 8);
    smallTitleFont.setBold(true);

    titleFont.setBold(true);
    QwtText title;
    title.setFont(titleFont);

    this->setMinimumHeight(325);
    this->setMinimumWidth(600);

    readRTTList();
    readCBRList();
    readCCList();
    ssh_killall  = ssh_killall + "&";
    _RUN_SCRIPT(ssh_killall);
    updateWebBrowsing0(true);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    mainLayout->setContentsMargins(10, 0, 10, 0);

    QGridLayout *dataDisplayLayout = new QGridLayout;
    displayNumDownloads = new QLabel("0", this);
    displayNumDownloads->setFixedWidth(20);
    QSlider *sliderDownloads = new QSlider(Qt::Vertical, this);
    sliderDownloads->setInvertedAppearance(true);
    sliderDownloads->setMinimum(0);
    sliderDownloads->setMaximum(10);
    QHBoxLayout *downloadsBoxLayout = new QHBoxLayout;
    QLabel *titleDownloads = new QLabel("Downloads", this);
    downloadsBoxLayout->addWidget(titleDownloads);
    rttSelect = new QComboBox(this);
    rttSelect->insertItems(0, rttList);
    rttSelect->setFixedWidth(50);
    QLabel *titleRTT = new QLabel("Add. RTT [ms]:", this);
    downloadsBoxLayout->addWidget(titleRTT);
    downloadsBoxLayout->addWidget(rttSelect);

    QHBoxLayout *webBrowsingBoxLayout = new QHBoxLayout;
    QLabel *titleWebBrowsing = new QLabel("Web browsing [Items/s]:", this);
    btnwb0 = new QRadioButton("0");
    btnwb0->setMaximumWidth(45);
    btnwb0->setChecked(true);
    btnwb10 = new QRadioButton("10");
    btnwb10->setMaximumWidth(45);
    btnwb100 = new QRadioButton("100");
    btnwb100->setMaximumWidth(45);
    webBrowsingBoxLayout->addWidget(titleWebBrowsing);
    webBrowsingBoxLayout->addWidget(btnwb0);
    webBrowsingBoxLayout->addWidget(btnwb10);
    webBrowsingBoxLayout->addWidget(btnwb100);

    plotWidget = new QwtPlot(this);
    barChart = new QwtPlotMultiBarChart("downloads");
    barChart->attach(plotWidget);
    plotWidget->setContentsMargins(-20,0,0,10);

    samples.resize(10);
    barChart->setSamples(samples);
    barChart->setOrientation(Qt::Horizontal);
    for (int i = 0 ; i < 10; ++i) {
        QwtColumnSymbol *symbol = new QwtColumnSymbol(QwtColumnSymbol::Box);
        symbol->setPalette(color);
        barChart->setSymbol(i, symbol);
    }

    noScaleDraw = new NoScaleDraw;
    plotWidget->setAxisScaleDraw(QwtPlot::yLeft, noScaleDraw);
    plotWidget->setAxisMaxMajor(QwtPlot::xBottom, 2);
    plotWidget->setAxisMaxMinor(QwtPlot::xBottom, 0);
    plotWidget->setAxisAutoScale(QwtPlot::xBottom, false);
    plotWidget->setAxisScale(QwtPlot::xBottom, 0.0, 200.0);
    unrelatedNumberScaleDraw = new UnrelatedNumberScaleDraw;
    unrelatedNumberScaleDraw->setNumber(6.6);
    plotWidget->setAxisScaleDraw(QwtPlot::xBottom, unrelatedNumberScaleDraw);

    QwtPlotMarker *marker = new QwtPlotMarker("limit");
    marker->setLineStyle(QwtPlotMarker::VLine);
    marker->setValue(100.0,0.0);
    marker->attach(plotWidget);

    plotWidgetAL = new QwtPlot(this);
    ALAppChart = new QwtPlotBarChart("AL");
    noScaleDrawAL = new NoScaleDraw;
    ALAppChart->attach(plotWidgetAL);
    sampleAL.push_back(0);
    ALAppChart->setSamples(sampleAL);
    ALAppChart->setSpacing(0);
    QHBoxLayout *alSelectLayout = new QHBoxLayout;
    QLabel *titleAL = new QLabel("AL", this);
    titleAL->setContentsMargins(0,0,0,0);
   /* alCheckb = new QCheckBox(this);
    alCheckb->setChecked(false);
    alSelectLayout->addWidget(alCheckb);*/
    alSelectLayout->addWidget(titleAL);

    QVBoxLayout *alccLayout = new QVBoxLayout;
    ccSelect = new QComboBox(this);
    ccSelect->insertItems(0, ccList);
    ccSelect->setFixedWidth(135);
    alccLayout->addWidget(plotWidgetAL);
    alccLayout->addWidget(ccSelect);

    QVBoxLayout *ccSelectLayout = new QVBoxLayout;
    QLabel *titleCC = new QLabel("CC", this);
    titleCC->setContentsMargins(0,0,0,0);
    ccSelectLayout->addWidget(titleCC);

    QVBoxLayout *alccSelectLayout = new QVBoxLayout;
    alccSelectLayout->addLayout(alSelectLayout);
    alccSelectLayout->addLayout(ccSelectLayout);

    ALAppChart->setOrientation(Qt::Horizontal);
    QwtColumnSymbol *symbol = new QwtColumnSymbol(QwtColumnSymbol::Box);
    symbol->setPalette(color);
    ALAppChart->setSymbol(symbol);
    plotWidgetAL->setAxisMaxMajor(QwtPlot::xBottom, 2);
    plotWidgetAL->setAxisMaxMinor(QwtPlot::xBottom, 0);
    plotWidgetAL->setAxisAutoScale(QwtPlot::xBottom, false);
    plotWidgetAL->setAxisScale(QwtPlot::xBottom, 0.0, 200.0);
    plotWidgetAL->setFixedHeight(15);
    plotWidgetAL->setContentsMargins(-25,0,0,-25);
    QwtPlotMarker *marker1 = new QwtPlotMarker("limit1");
    marker1->setLineStyle(QwtPlotMarker::VLine);
    marker1->setValue(100.0,0.0);
    marker1->attach(plotWidgetAL);

    QwtText titleCBRX;
    titleCBRX.setFont(smallTitleFont);
    titleCBRX.setText("BW %");

    plotWidgetCBR = new QwtPlot(this);
    CBRAppChart = new QwtPlotBarChart("CBR");
    noScaleDrawCBR = new NoScaleDraw;
    CBRAppChart->attach(plotWidgetCBR);
    sampleCBR.push_back(0);
    CBRAppChart->setSamples(sampleCBR);
    CBRAppChart->setSpacing(0);
    QVBoxLayout *cbrSelectLayout = new QVBoxLayout;
    QLabel *titleCBR = new QLabel("CBR", this);
    titleCBR->setContentsMargins(0,0,0,0);
    cbrSelect = new QComboBox(this);
    cbrSelect->insertItems(0, cbrList);
    cbrSelect->setFixedWidth(50);
    cbrSelectLayout->addWidget(titleCBR);
    cbrSelectLayout->addWidget(cbrSelect);
    CBRAppChart->setOrientation(Qt::Horizontal);

    QHBoxLayout *cbrLayout = new QHBoxLayout;
    cbrLayout->addLayout(cbrSelectLayout);
    cbrLayout->addWidget(plotWidgetCBR);

    QwtColumnSymbol *symbolcbr = new QwtColumnSymbol(QwtColumnSymbol::Box);
    symbolcbr->setPalette(color);
    CBRAppChart->setSymbol(symbolcbr);
    plotWidgetCBR->setAxisScaleDraw(QwtPlot::yLeft, noScaleDrawCBR);
    plotWidgetCBR->setAxisMaxMajor(QwtPlot::xBottom, 2);
    plotWidgetCBR->setAxisMaxMinor(QwtPlot::xBottom, 0);
    plotWidgetCBR->setAxisAutoScale(QwtPlot::xBottom, false);
    plotWidgetCBR->setAxisScale(QwtPlot::xBottom, 0.0, 100.0);
    plotWidgetCBR->setMinimumHeight(58);
    plotWidgetCBR->setContentsMargins(-25,0,0,0);
    plotWidgetCBR->setAxisTitle(2, titleCBRX);

    plotCompl = new QwtPlot(this);
    scatterCompl = new QwtPlotSpectroCurve("compl");
    scatterCompl->attach(plotCompl);

    scatterCompl->setPenWidth(4);
    QColor b("blue");
    QwtAlphaColorMap *colormap = new QwtAlphaColorMap(color);
    scatterCompl->setColorMap(colormap);
    plotCompl->setMinimumHeight(200);

    logscaleX = new QwtLogScaleEngine();
    logscaleY = new QwtLogScaleEngine();

    plotCompl->setAxisScaleEngine(0,(QwtScaleEngine*)logscaleY);
    plotCompl->setAxisScaleEngine(2,(QwtScaleEngine*)logscaleX);
    plotCompl->setAxisFont(0, axisFont);
    plotCompl->setAxisFont(2, axisFont);
    title.setText("Flow size [log(bytes)]");
    plotCompl->setAxisTitle(2, title);
    title.setText("Time [log(sec)]");
    plotCompl->setAxisTitle(0, title);
    plotCompl->setAxisScale(2, 1000, 1000000);
    plotCompl->setAxisScale(0, 0.01, 10);

    dataDisplayLayout->addWidget(displayNumDownloads, 0, 0);
    dataDisplayLayout->addLayout(downloadsBoxLayout, 0, 1, 1, 1);
    dataDisplayLayout->addLayout(webBrowsingBoxLayout, 0, 2, 1, 1);
    dataDisplayLayout->addWidget(sliderDownloads, 1, 0, 1, 1);
    dataDisplayLayout->addWidget(plotWidget, 1, 1, 1, 1);
    dataDisplayLayout->addWidget(plotCompl, 1, 2, 1, 1);
    dataDisplayLayout->addLayout(alccLayout, 3,1,1,1);
    dataDisplayLayout->addLayout(alccSelectLayout, 3,0,1,1);
    dataDisplayLayout->addLayout(cbrLayout, 3,2,1,1);
    alccSelectLayout->setAlignment(titleAL, Qt::AlignTop);
    alccSelectLayout->setAlignment(titleCC, Qt::AlignBottom);
    alccLayout->setAlignment(plotWidgetAL, Qt::AlignTop);
    alccLayout->setAlignment(ccSelect, Qt::AlignBottom);

    mainLayout->addLayout(dataDisplayLayout);

    connect(this, SIGNAL(scheduleReplot()), this, SLOT(commitData()));
    connect(sliderDownloads, SIGNAL(sliderMoved(int)), this, SLOT(updateNumDownloads(int)));
    connect(sliderDownloads, SIGNAL(sliderReleased()), this, SLOT(startDownloads()));
    connect(btnwb0, SIGNAL(toggled(bool)), this, SLOT(updateWebBrowsing0(bool)));
    connect(btnwb10, SIGNAL(toggled(bool)), this, SLOT(updateWebBrowsing100(bool)));
    connect(btnwb100, SIGNAL(toggled(bool)), this, SLOT(updateWebBrowsing10(bool)));
    connect(rttSelect, SIGNAL(currentIndexChanged(int)), this, SLOT(updateRTT(int)));
  //  connect(alCheckb, SIGNAL(toggled(bool)), this, SLOT(updateAL(bool)));
    connect(cbrSelect, SIGNAL(currentIndexChanged(int)), this, SLOT(updateCBR(int)));
    connect(ccSelect, SIGNAL(currentIndexChanged(int)), this, SLOT(updateCC(int)));
}

Client::~Client()
{
    _RUN_SCRIPT(ssh_killall);
}

void Client::updateNumDownloads(int num)
{
    nrFlows=num;
    displayNumDownloads->setText(QString().setNum(num));
}

void Client::startDownloads()
{
    std::stringstream command;
    command << ssh_download << " "<<  nrFlows << " &";
    _RUN_SCRIPT(ssh_killdownload);
    _RUN_SCRIPT(command.str());
}

void Client::updateSamples(std::vector<double> rsamples, double cbr_rate, double al_rate,
                           QVector<QwtPoint3D>& csamples, QVector<QwtPoint3D>& csamples_hs)
{
    QMutexLocker m(&dataMutex);
    samples.clear();
    samples.resize(10);

    int i = 0;
    for (auto it = rsamples.begin(); it != rsamples.end(); ++it){
          samples[9-i].push_back(*it);
          i++;
          if (i > 9)
              break;
    }
    sampleCBR.replace(0, cbr_rate);
    sampleAL.replace(0, al_rate);

    int nr = 0;
    for (auto it = csamples.begin(), it_hs = csamples_hs.begin();
         it != csamples.end(), it_hs != csamples_hs.end(); ++it, ++it_hs){
        CData.append(*it);
        CData_hs.append(*it_hs);
        nr++;
    }
    nrComplSamples.push_back(nr);

    if (nrComplSamples.size() > 60){
        nr = nrComplSamples.front();
        nrComplSamples.erase(nrComplSamples.begin());
        for (int i = 0; i < nr; ++i) {
            CData.erase(CData.begin());
            CData_hs.erase(CData_hs.begin());
        }
    }
    double subtract = 16;
    int index = CData.size()-1-nrComplSamples.back();
    for (auto itNr = nrComplSamples.crbegin()+1; itNr != nrComplSamples.crend(); ++itNr) {
        while (index >= 0 && index > index-*itNr) {
            QwtPoint3D point = CData.at(index);
            point.setZ(point.z()-subtract);
            CData.replace(index,point);
            QwtPoint3D point_hs = CData_hs.at(index);
            point_hs.setZ(point_hs.z()-subtract);
            CData_hs.replace(index,point_hs);
            index--;
         }
         subtract += 16;
    }
    scheduleReplot();
}

void Client::updateFairRate(double r, QString c)
{
    QMutexLocker m(&dataMutex);
    rate = r;
    caption = c;
}

void Client::commitData()
{
    QMutexLocker m(&dataMutex);
    barChart->setSamples(samples);

    unrelatedNumberScaleDraw->setNumber(rate);
    unrelatedNumberScaleDraw->setUnit(caption);

    plotWidget->replot();

    ALAppChart->setSamples(sampleAL);
    plotWidgetAL->replot();
    CBRAppChart->setSamples(sampleCBR);
    plotWidgetCBR->replot();

    QwtScaleWidget *scaleWidget = plotWidget->axisWidget(QwtPlot::xBottom);
    if (scaleWidget) scaleWidget->repaint();
    QwtScaleWidget *scaleWidgetAL = plotWidgetAL->axisWidget(QwtPlot::xBottom);
    if (scaleWidgetAL) scaleWidgetAL->repaint();
    QwtScaleWidget *scaleWidgetCBR = plotWidgetCBR->axisWidget(QwtPlot::xBottom);
    if (scaleWidgetCBR) scaleWidgetCBR->repaint();

    if (complHS)
        scatterCompl->setSamples(CData_hs);
    else
        scatterCompl->setSamples(CData);
    plotCompl->replot();
}

void Client::updateWebBrowsing0(bool toggled)
{
    if (toggled) {
        std::string command = ssh_wb;
        command = command + "0 &";
        _RUN_SCRIPT(command);
    }
}

void Client::updateWebBrowsing10(bool toggled)
{
    if (toggled) {
        std::stringstream command;
        command << ssh_wb << "10 " << getLinkCap() << " &";
        _RUN_SCRIPT(command.str());
    }
}

void Client::updateWebBrowsing100(bool toggled)
{
    if (toggled) {
        std::stringstream command;
        command << ssh_wb << "100 " << getLinkCap() << " &";
        _RUN_SCRIPT(command.str());
    }
}

void Client::readRTTList(){
    std::ifstream infile(res_path("/config/ertt_list"));
    if (infile.is_open()) {
        std::string rtt;
        int rttValue = -1;
        while (!infile.eof()) {
            infile >> rtt;
            if (atoi(rtt.c_str()) != rttValue) {
                rttValue = atoi(rtt.c_str());
                rttList.append(rtt.c_str());
                rttValues.push_back(rttValue);
            }
        }
        infile.close();
    } else
        std::cerr << "ERROR: 'rtt_list' file is missing or corrupted." << std::endl;
}

void Client::readCBRList(){
    std::string cbr;
    int cbrValue = 0;
    cbrList.append("off");
    cbrValues.push_back(cbrValue);

    std::ifstream infile(res_path("/config/cbr_list"));
    if (infile.is_open()) {
       while (!infile.eof()) {
            infile >> cbr;
            if (atoi(cbr.c_str()) != cbrValue) {
                cbrValue = atoi(cbr.c_str());
                cbrList.append(cbr.c_str());
                cbrValues.push_back(cbrValue);
            }
        }
        infile.close();
    } else
        std::cerr << "ERROR: 'cbr_list' file is missing or corrupted." << std::endl;
}

void Client::readCCList()
{
    std::ifstream infile(res_path("/config/cc_list"));
    if (infile.is_open()) {
        std::string cc_name;
        std::string cc_command;
        while (!infile.eof()){
            std::stringstream ss_cc_name;
            infile >> cc_name;
            ss_cc_name << cc_name;
            infile >> cc_name;
            while (!infile.eof() && cc_name != ":") {
                ss_cc_name << " " << cc_name;
                infile >> cc_name;
            }
            if (!infile.eof()) {
                infile >> cc_command;
                ccList.append(ss_cc_name.str().c_str());
                ccValues.push_back(cc_command);
            }
        }
        infile.close();
    } else
        std::cerr << "ERROR: 'cc_list' file is missing or corrupted." << std::endl;
}

void Client::updateRTT(int num)
{
    std::stringstream command;
    command << ssh_rtt << " " << rttValues.at(num);
    _RUN_SCRIPT(command.str());
    rttChanged(rttValues.at(num));
}

void Client::clearComplData()
{
    QMutexLocker m(&dataMutex);
    CData.clear();
    CData_hs.clear();
    nrComplSamples.clear();
    if (complHS)
        scatterCompl->setSamples(CData_hs);
    else
        scatterCompl->setSamples(CData);
    plotCompl->replot();
}

void Client::updateComplHS(bool value)
{
    QMutexLocker m(&dataMutex);
    complHS = value;
    if (complHS)
        scatterCompl->setSamples(CData_hs);
    else
        scatterCompl->setSamples(CData);

}

void Client::setComplHS(bool value)
{
    if (value == true)
        updateComplHS(true);
}

void Client::setComplWOHS(bool value)
{
    if (value == true)
        updateComplHS(false);
}

void Client::updateAL(bool value)
{
    std::stringstream command;
    command << ssh_al << " " << int(value) << "&";
    _RUN_SCRIPT(command.str());
}

void Client::updateCBR(int value)
{
    std::stringstream command;
    command << ssh_cbr << " " << getCBRRate(value) << "&";
    _RUN_SCRIPT(command.str());
}

int Client::getCBRRate(int value)
{
    QMutexLocker m(&linkcapMutex);
    int rate = linkcap*cbrValues.at(value)/100;
    return rate;
}

void Client::updateLinkCap(int value)
{
    QMutexLocker m(&linkcapMutex);
    linkcap = value;
}

void Client::updateCC(int value)
{
    if (ccValues.size() < value+1)
        return;
    if (ccSelect->currentIndex() != value)
        ccSelect->setCurrentIndex(value);
    std::stringstream command;
    _RUN_SCRIPT(ssh_killall);
    command << ssh_cc << " " << ccValues.at(value);
    _RUN_SCRIPT(command.str());

    ccChanged(value);
    startDownloads();
    if (btnwb10->isChecked()) {
        std::stringstream command;
        command << ssh_wb << "10 " << getLinkCap() << " &";
        _RUN_SCRIPT(command.str());
    } else if (btnwb100->isChecked()) {
        std::stringstream command;
        command << ssh_wb << "100 " << getLinkCap() << " &";
        _RUN_SCRIPT(command.str());
    }

    int cbrrate = getCBRRate(cbrSelect->currentIndex());
    if (cbrrate > 0) {
        std::stringstream command;
        command << ssh_cbr << " " << getCBRRate(value) << "&";
        _RUN_SCRIPT(command.str());
    }
   /* if (alCheckb->isChecked()) {
        std::stringstream command;
        command << ssh_al << " " << int(value) << "&";
        _RUN_SCRIPT(command.str());
    }*/
}

int Client::getLinkCap()
{
    QMutexLocker m(&linkcapMutex);
    return linkcap;
}
