#-------------------------------------------------
#
# Project created by QtCreator 2017-08-24T14:24:54
#
#-------------------------------------------------

QT       += network concurrent

QT       -= gui

TARGET = networking
TEMPLATE = lib
CONFIG += staticlib c++14

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

include($$PWD/../../modules/qtpromise/qtpromise.pri)
include($$PWD/../common/libcommon.pri)

INCLUDEPATH += $$PWD/vendor/include

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    Networking/AbstractBlockExplorerHttpClient.cpp \
    Networking/RequestHandlerImpl.cpp \
    Networking/XSNBlockExplorerHttpClient.cpp \
    Networking/NetworkingUtils.cpp \
    Networking/AbstractRemotePriceProvider.cpp \
    Networking/CMCRemotePriceProvider.cpp

HEADERS += \
    Networking/RequestHandlerImpl.hpp \
    Networking/AbstractBlockExplorerHttpClient.hpp \
    Networking/XSNBlockExplorerHttpClient.hpp \
    Networking/NetworkingUtils.hpp \
    Networking/AbstractRemotePriceProvider.hpp \
    Networking/CMCRemotePriceProvider.hpp


unix {
    target.path = /usr/lib
    INSTALLS += target
}

DISTFILES += \
    libnetworking.pri
