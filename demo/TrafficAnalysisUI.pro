#-------------------------------------------------
#
# Project created by QtCreator 2015-07-08T05:11:23
#
#-------------------------------------------------

QT       += core gui widgets

TARGET = L4SDemo
CONFIG   -= app_bundle
CONFIG += c++11 release force_debug_info
TEMPLATE = app


SOURCES += src/main.cpp \
    src/datagenerator.cpp \
    src/mainwindow.cpp \
    src/trafficplot.cpp \
    src/client.cpp \
    src/trafficanalyzer.cpp \
    src/historyplot.cpp \
    src/filledcurve.cpp \
    src/curve.cpp \
    src/compltimesocket.cpp \
    src/linkaqm.cpp \
    src/demodata.cpp \
    src/trafficanalyzerstat.cpp \
    src/script_runner.cpp

HEADERS += \
    src/datagenerator.h \
    src/mainwindow.h \
    src/trafficplot.h \
    src/client.h \
    src/trafficanalyzer.h \
    src/historyplot.h \
    src/filledcurve.h \
    src/curve.h \
    src/compltimesocket.h \
    src/linkaqm.h \
    src/demodata.h \
    src/trafficanalyzerstat.h \
    src/resources.h \
    src/script_runner.h

exists(/usr/local/qwt-6.1.4) {
    INCLUDEPATH += /usr/local/qwt-6.1.4/include
    LIBS += -L/usr/local/qwt-6.1.4/lib -lqwt
} else {
    exists(/usr/include/qwt) {
        INCLUDEPATH += /usr/include/qwt
        LIBS += -lqwt
    } else {
        INCLUDEPATH += ../../Qt5.0.1/qwt-6.1.4/src
        LIBS += -L../../Qt5.0.1/qwt-6.1.4/lib -lqwt
    }
}

INCLUDEPATH += ../traffic_analyzer
INCLUDEPATH += ../common
LIBS += -L../traffic_analyzer -lta -lpcap

@QMAKE_STRIP=
#QMAKE_POST_LINK += sh/setcap.sh
QMAKE_LFLAGS += -rdynamic

DISTFILES += \
    config/aqm_list \
    config/brtt_list \
    config/cbr_list \
    config/cc_list \
    config/ertt_list \
    config/link_cap_list \
    sh/__ssh_lib.sh \
    sh/al_cubic.sh \
    sh/al_dctcp.sh \
    sh/cbr_cubic.sh \
    sh/cbr_dctcp.sh \
    sh/cc_cubic.sh \
    sh/cc_dctcp.sh \
    sh/check_if_up.sh \
    sh/killall_cubic.sh \
    sh/killall_dctcp.sh \
    sh/killdownload_cubic.sh \
    sh/killdownload_dctcp.sh \
    sh/rtt_cubic.sh \
    sh/rtt_dctcp.sh \
    sh/set_aqm_link.sh \
    sh/set_tcp_cc.sh \
    sh/setcap.sh \
    sh/start_cubic_download.sh \
    sh/start_dctcp_download.sh \
    sh/wb_cubic.sh \
    sh/wb_dctcp.sh
