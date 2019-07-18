#include "mainwindow.h"
#include "resources.h"

#include "client.h"
#include "linkaqm.h"
#include "datagenerator.h"
#include "script_runner.h"

#include <QLayout>

#include <QPushButton>
#include <QLabel>
#include <QRadioButton>
#include <QHBoxLayout>
#include <QPushButton>
#include <iostream>
#include <sstream>
#include <cstdlib>

MainWindow::MainWindow(QWidget *parent) : QWidget(parent)
{
    _RUN_SCRIPT(res_path("/sh/prepare_endhosts.sh"));
    Client *dctcpclient = new Client(this,
		    res_path("/sh/dctcp_download.sh").c_str(),
		    res_path("/sh/killall_dctcp.sh").c_str(),
		    res_path("/sh/wb_dctcp.sh ").c_str(),
		    res_path("/sh/rtt_dctcp.sh").c_str(),
		    res_path("/sh/cc_dctcp.sh").c_str(),
		    /* res_path("/sh/al_dctcp.sh").c_str(), */
		    res_path("/sh/cbr_dctcp.sh").c_str(),
		    Qt::blue, 0);
    Client *cubicclient = new Client(this,
		    res_path("/sh/cubic_download.sh").c_str(),
		    res_path("/sh/killall_cubic.sh").c_str(),
		    res_path("/sh/wb_cubic.sh ").c_str(),
		    res_path("/sh/rtt_cubic.sh").c_str(),
		    res_path("/sh/cc_cubic.sh").c_str(),
		    /* res_path("/sh/al_cubic.sh").c_str(), */
		    res_path("/sh/cbr_cubic.sh").c_str(),
		    QColor(255, 157, 0), 1);

    QHBoxLayout *mainLayout = new QHBoxLayout;
    setLayout(mainLayout);

    QVBoxLayout *firstColumn = new QVBoxLayout;
    mainLayout->addLayout(firstColumn);

    std::string client_a = safe_getenv("CLIENT_A");
    std::string server_a = safe_getenv("SERVER_A");
    std::string client_b = safe_getenv("CLIENT_B");
    std::string server_b = safe_getenv("SERVER_B");
    std::stringstream dctcpTitle, cubicTitle;
    dctcpTitle
	    << "$CLIENT_A [" << client_a
	    << "] <> $SERVER_A [" << server_a << "]";
    cubicTitle
	    << "$CLIENT_B [" << client_b
	    << "] <> $SERVER_B [" << server_b << "]";
    dctcpclient->setTitle(dctcpTitle.str().c_str());
    firstColumn->addWidget(dctcpclient);
    firstColumn->setAlignment(dctcpclient, Qt::AlignTop);
    cubicclient->setTitle(cubicTitle.str().c_str());
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
    std::string pcapf = safe_getenv("PCAPFILTER");
    std::string dev = safe_getenv("IFACE");
    std::stringstream laqm_title;
    laqm_title
	    << "Monitoring $IFACE[" << dev << "] "
	    << "with $PCAPFILTER[" << pcapf << "]"
	    << std::endl;
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
    g = new DataGenerator(dctcpclient, cubicclient, laqm, dev, pcapf);
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
    connect(dctcpclient, SIGNAL(ccChanged(std::string)), this, SLOT(updateDctcpclientCC(std::string)));
    connect(cubicclient, SIGNAL(ccChanged(std::string)), this, SLOT(updateCubicclientCC(std::string)));

    connect(becn, SIGNAL(toggled(bool)), g, SLOT(setECNClass(bool)));
    connect(bip, SIGNAL(toggled(bool)), g, SLOT(setIPClass(bool)));

    generatorThread->start();
}

MainWindow::~MainWindow()
{
	_RUN_SCRIPT(res_path("/sh/kill_ssh.sh"));
	delete g;
	ScriptRunner::instance().stop();
}

void MainWindow::updateDctcpclientCC(std::string name)
{
    QMutexLocker m(&dataMutex);
    dctcpclientCC = name;
    /* checkCC(); */
}

void MainWindow::updateCubicclientCC(std::string name)
{
    QMutexLocker m(&dataMutex);
    cubicclientCC = name;
    /* checkCC(); */
}

void MainWindow::checkCC()
{
    if ((dctcpclientCC == "dctcp" || dctcpclientCC == "cubic_ecn" ||
	 dctcpclientCC == "prague") &&
	(cubicclientCC == "cubic" || cubicclientCC == "reno" ||
	 cubicclientCC == "bbr")) {
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
