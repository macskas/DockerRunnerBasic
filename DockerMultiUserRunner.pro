TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    cdockercontainer.cpp \
    cdocker.cpp \
    cphprunner.cpp

HEADERS += \
    cdockercontainer.h \
    cdocker.h \
    globals.h \
    cphprunner.h

DISTFILES += \
    Makefile

