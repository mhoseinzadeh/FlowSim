#-------------------------------------------------
#
# Project created by QtCreator 2017-08-04T17:26:04
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = flowsim
TEMPLATE = app

CONFIG += c++14

DEFINES += \
    CYCLE_WIDTH=15

SOURCES += main.cpp\
        mainwindow.cpp \
    block.cpp \
    flow.cpp \
    bst.cpp \
    graphicsscene.cpp \
    element.cpp \
    sample1.cpp

HEADERS  += mainwindow.h \
    block.h \
    flow.h \
    bst.h \
    graphicsscene.h \
    element.h \
    fiboheap.h \
    fiboqueue.h \
    sample1.h

FORMS    += mainwindow.ui

RESOURCES += \
    res.qrc

DISTFILES +=

QMAKE_CFLAGS_RELEASE -= -O2
QMAKE_CFLAGS_RELEASE *= -O3

QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE *= -O3
