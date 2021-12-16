TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG += qt

#include($$PWD/../../modules/grpc/grpc.pri)

include($$PWD/../lndtools/lndtools.pri)

#INCLUDEPATH += $$PWD/../lightningswapsdaemon

SOURCES += \
        main.cpp

HEADERS += \

LIBS += -lz -pthread
