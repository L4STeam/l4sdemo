#-------------------------------------------------
#
# Project created by QtCreator 2015-07-08T05:11:23
#
#-------------------------------------------------

QT       += core gui widgets

TARGET = L4SDemo
CONFIG   -= app_bundle
CONFIG += c++11
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
    src/trafficanalyzerstat.cpp

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
    src/trafficanalyzerstat.h

INCLUDEPATH += ../../Qt/qwt-6.1/src
INCLUDEPATH += ../traffic_analyzer

LIBS += -L../../Qt/qwt-6.1/lib -lqwt
LIBS += -L../traffic_analyzer -lta -lpcap

