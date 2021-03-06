#-------------------------------------------------
#
# Project created by QtCreator 2019-08-08T12:36:03
#
#-------------------------------------------------

QT       -= gui

TARGET = common
TEMPLATE = lib
CONFIG += staticlib

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

include($$PWD/../../modules/qtpromise/qtpromise.pri)

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0



unix {
    target.path = /usr/lib
    INSTALLS += target
}

HEADERS += \
    Utils/ExtendedKeyPathBip44.hpp \
    Utils/Logging.hpp \
    Utils/Utils.hpp

SOURCES += \
    Utils/ExtendedKeyPathBip44.cpp \
    Utils/Logging.cpp \
    Utils/Utils.cpp

DISTFILES += \
    libcommon.pri
