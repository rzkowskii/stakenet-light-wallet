QT -= gui
QT += network

CONFIG -= app_bundle
TARGET = tor
TEMPLATE = lib
CONFIG += staticlib c++14

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

include($$PWD/../bitcoin/libbitcoin.pri)
include($$PWD/../common/libcommon.pri)
include($$PWD/../../modules/libtor/libtor.pri)

android {
    include($$PWD/../../modules/liblzma/liblzma.pri)
}

INCLUDEPATH += \
    $$PWD/../../depends/tor/sources/src

SOURCES += \
    TorController.cpp

HEADERS += \
    TorController.hpp

DISTFILES += \
    tor.pri
