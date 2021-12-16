#-------------------------------------------------
#
# Project created by QtCreator 2019-09-17T20:44:33
#
#-------------------------------------------------

QT       -= gui

TARGET = qgrpc
TEMPLATE = lib
CONFIG += staticlib c++14

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

include($$PWD/../common/libcommon.pri)
include($$PWD/../../modules/grpc/grpc.pri)
include($$PWD/../../modules/boost/libboost.pri)

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

HEADERS += \ \
    GRPCTools/ClientUtils.hpp \
    GRPCTools/ServerUtils.hpp

unix {
    target.path = /usr/lib
    INSTALLS += target
}

DISTFILES += \
    grpc.pri

SOURCES += \
    GRPCTools/ClientUtils.cpp \
    GRPCTools/ServerUtils.cpp
