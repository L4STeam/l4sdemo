#include "mainwindow.h"
#include "resources.h"

#include "client.h"
#include "linkaqm.h"
#include "datagenerator.h"


#include <QLayout>

#include <QPushButton>
#include <QLabel>
#include <QRadioButton>
#include <QHBoxLayout>
#include <QPushButton>
#include <iostream>
#include <sstream>
#include <cstdlib>

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
    , dctcpclientCC(0)
    , cubicclientCC(1)
{
    QHBoxLayout *mainLayout = new QHBoxLayout;
    setLayout(mainLayout);

    QVBoxLayout *firstColumn = new QVBoxLayout;
    mainLayout->addLayout(firstColumn);
    std::stringstream dctcpTitle, cubicTitle;

    std::string client_a = safe_getenv("CLIENT_A");
    std::string server_a = safe_getenv("SERVER_A");
    std::string client_b = safe_getenv("CLIENT_B");
    std::string server_b = safe_getenv("SERVER_B");
    std::string pcapf = safe_getenv("PCAPFILTER");
    std::string dev = safe_getenv("IFACE");
    std::cout
	    << "Client A: " << client_a
	    << "Server A: " << server_a
	    << "Client B: " << client_b
	    << "Server B: " << server_b
	    << "PCAPFILTER: " << pcapf
	    << "INTERFACE: " << dev
	    << std::endl;
    dctcpTitle
	    << "$CLIENT_A [" << client_a
	    << "] <> $SERVER_A [" << server_a << "]";
    cubicTitle
	    << "$CLIENT_B [" << client_b
	    << "] <> $SERVER_B [" << server_b << "]";

    Client *dctcpclient = new Client(this,
		    res_path("/sh/start_dctcp_download.sh").c_str(),
		    res_path("/sh/killall_dctcp.sh").c_str(),
		    res_path("/sh/killdownload_dctcp.sh").c_str(),
		    res_path("/sh/wb_dctcp.sh ").c_str(),
		    res_path("/sh/rtt_dctcp.sh").c_str(),
		    res_path("/sh/cc_dctcp.sh").c_str(),
		    res_path("/sh/al_dctcp.sh").c_str(),
		    res_path("/sh/cbr_dctcp.sh").c_str(), Qt::blue);
    dctcpclient->setTitle(dctcpTitle.str().c_str());
    dctcpclient->updateCC(0);
    firstColumn->addWidget(dctcpclient);
    firstColumn->setAlignment(dctcpclient, Qt::AlignTop);
    Client *cubicclient = new Client(this,
		    res_path("/sh/start_cubic_download.sh").c_str(),
		    res_path("/sh/killall_cubic.sh").c_str(),
		    res_path("/sh/killdownload_cubic.sh").c_str(),
		    res_path("/sh/wb_cubic.sh ").c_str(),
		    res_path("/sh/rtt_cubic.sh").c_str(),
		    res_path("/sh/cc_cubic.sh").c_str(),
		    res_path("/sh/al_cubic.sh").c_str(),
		    res_path("/sh/cbr_cubic.sh").c_str(), QColor(255, 157, 0));
    cubicclient->setTitle(cubicTitle.str().c_str());
    cubicclient->updateCC(1);
    firstColumn->addWidget(cubicclient);
    firstColumn->setAlignment(cubicclient, Qt::AlignBottom);

    QHBoxLayout *optionsLayout = new QHBoxLayout;
    QGroupBox *toggleRWBox = new QGroupBox;
    toggleRWBox->setMaximumHeight(60);
    QHBoxLayout *toggleRWBoxLayout = new QHBoxLayout;
    toggleRWBox->setLayout(toggleRWBoxLayout);
    QRadioButton *br = new QRadioButton("Rate");
    br->setChecked(true);
    QRadioButton *bw = new QRadioButton("Window");
    toggleRWBoxLayout->addWidget(br);
    toggleRWBoxLayout->addWidget(bw);
    optionsLayout->addWidget(toggleRWBox);

    QHBoxLayout *complOptLayout = new QHBoxLayout;
    QGroupBox *toggleComplHSBox = new QGroupBox;
    toggleComplHSBox->setMaximumHeight(60);
    QHBoxLayout *toggleComplHSLayout = new QHBoxLayout;
    toggleComplHSBox->setLayout(toggleComplHSLayout);
    QRadioButton *whs = new QRadioButton("w/HS");
    whs->setChecked(true);
    QRadioButton *wohs = new QRadioButton("wo/HS");
    toggleComplHSLayout->addWidget(whs);
    toggleComplHSLayout->addWidget(wohs);
    complOptLayout->addWidget(toggleComplHSBox);

    QPushButton *clearCompl = new QPushButton("Clear", this);
    complOptLayout->addWidget(clearCompl);
    clearCompl->setMinimumHeight(40);
    optionsLayout->addLayout(complOptLayout);
    complOptLayout->setAlignment(clearCompl, Qt::AlignBottom);
    optionsLayout->setAlignment(complOptLayout, Qt::AlignRight);



    firstColumn->addLayout(optionsLayout);
    firstColumn->setAlignment(optionsLayout, Qt::AlignBottom);

    // second column
    QVBoxLayout *secondColumn = new QVBoxLayout;
    mainLayout->addLayout(secondColumn);
    Linkaqm *laqm = new Linkaqm();
    std::stringstream laqm_title;
    laqm_title
	    << "Monitoring interface [" << dev << "] "
	    << "with filter [" << pcapf << "]"
	    <<std::endl;
    laqm->setTitle(laqm_title.str().c_str());
    secondColumn->addWidget(laqm);


    QGroupBox *toggleIPClassBox = new QGroupBox;
    toggleIPClassBox->setMaximumHeight(60);
    toggleIPClassBox->setMaximumWidth(120);

    QHBoxLayout *toggleIPClassLayout = new QHBoxLayout;
    toggleIPClassBox->setLayout(toggleIPClassLayout);
    becn = new QRadioButton("ECN");
    becn->setChecked(true);
    bip = new QRadioButton("IP");
    toggleIPClassLayout->addWidget(becn);
    toggleIPClassLayout->addWidget(bip);
    secondColumn->addWidget(toggleIPClassBox);
    secondColumn->setAlignment(toggleIPClassBox, Qt::AlignRight);


    QThread *generatorThread = new QThread();
    DataGenerator *g = new DataGenerator(dctcpclient, cubicclient, laqm,
					 dev, pcapf);
    g->moveToThread(generatorThread);
    g->startCompl();
    connect(generatorThread, SIGNAL(started()), g, SLOT(startTA()));

    connect(br, SIGNAL(toggled(bool)), g, SLOT(setShowRate(bool)));
    connect(bw, SIGNAL(toggled(bool)), g, SLOT(setShowWindow(bool)));
    connect(clearCompl, SIGNAL(pressed()), dctcpclient, SLOT(clearComplData()));
    connect(clearCompl, SIGNAL(pressed()), cubicclient, SLOT(clearComplData()));
    connect(whs, SIGNAL(toggled(bool)), dctcpclient, SLOT(setComplHS(bool)));
    connect(whs, SIGNAL(toggled(bool)), cubicclient, SLOT(setComplHS(bool)));
    connect(wohs, SIGNAL(toggled(bool)), dctcpclient, SLOT(setComplWOHS(bool)));
    connect(wohs, SIGNAL(toggled(bool)), cubicclient, SLOT(setComplWOHS(bool)));
    connect(laqm, SIGNAL(linkChanged(int)), g, SLOT(updateLinkCap(int)));
    connect(laqm, SIGNAL(linkChanged(int)), dctcpclient, SLOT(updateLinkCap(int)));
    connect(laqm, SIGNAL(linkChanged(int)), cubicclient, SLOT(updateLinkCap(int)));
    connect(this, SIGNAL(updateTA(bool)), g, SLOT(startTA(bool)));
    connect(dctcpclient, SIGNAL(ccChanged(int)), this, SLOT(updateDctcpclientCC(int)));
    connect(cubicclient, SIGNAL(ccChanged(int)), this, SLOT(updateCubicclientCC(int)));

    connect(becn, SIGNAL(toggled(bool)), g, SLOT(setECNClass(bool)));
    connect(bip, SIGNAL(toggled(bool)), g, SLOT(setIPClass(bool)));

    generatorThread->start();
}

void MainWindow::updateDctcpclientCC(int value)
{
    QMutexLocker m(&dataMutex);
    dctcpclientCC = value;
    /* checkCC(); */
}

void MainWindow::updateCubicclientCC(int value)
{
    QMutexLocker m(&dataMutex);
    cubicclientCC = value;
    /* checkCC(); */
}

void MainWindow::checkCC()
{
    if ((dctcpclientCC == 0 || dctcpclientCC == 2 )&& (cubicclientCC == 1 || cubicclientCC == 3)) {
        if (!becn->isChecked())
            becn->toggle();
        std::cout << "set checked ECN" << std::endl;
    } else {
        if (!bip->isChecked())
            bip->toggle();
        std::cout << "set checked IP" << std::endl;

    }
}

int MainWindow::checkIfUp(const char* ip)
{
    std::string command = res_path("/sh/check_if_up.sh");
    command = command + " " + ip;
    int res = system(command.c_str());
    return res/256;
}
