TEMPLATE = app
CONFIG += console c++14
CONFIG -= app_bundle
CONFIG += qt

include($$PWD/../../modules/grpc/grpc.pri)

INCLUDEPATH += $$PWD/../lightningswapsdaemon

SOURCES += \
        ../lightningswapsdaemon/Protos/lssdrpc.grpc.pb.cc \
        ../lightningswapsdaemon/Protos/lssdrpc.pb.cc \
        main.cpp

DISTFILES += \ \
    Protos/lssdrpc.proto

HEADERS += \
    ../lightningswapsdaemon/Protos/lssdrpc.grpc.pb.h \
    ../lightningswapsdaemon/Protos/lssdrpc.pb.h

LIBS += -lz -pthread
