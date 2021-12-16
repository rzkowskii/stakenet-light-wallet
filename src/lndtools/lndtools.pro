#-------------------------------------------------
#
# Project created by QtCreator 2019-09-17T20:44:33
#
#-------------------------------------------------

QT       -= gui

TARGET = lndtools
TEMPLATE = lib
CONFIG += staticlib c++14

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

include($$PWD/../common/libcommon.pri)
include($$PWD/../../modules/boost/libboost.pri)
include($$PWD/../qgrpc/qgrpc.pri)


# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


unix {
    target.path = /usr/lib
    INSTALLS += target
}

DISTFILES += \
    LndTools/Protos/Common.proto \
    LndTools/Protos/KeychainService.proto \
    LndTools/Protos/LightWalletService.proto \
    LndTools/Protos/README.md \
    LndTools/Protos/autopilot.proto \
    LndTools/Protos/google/api/annotations.proto \
    LndTools/Protos/google/api/http.proto \
    LndTools/Protos/google/api/httpbody.proto \
    LndTools/Protos/google/rpc/code.proto \
    LndTools/Protos/google/rpc/error_details.proto \
    LndTools/Protos/google/rpc/status.proto \
    LndTools/Protos/invoices.proto \
    LndTools/Protos/router.proto \
    LndTools/Protos/rpc.proto

HEADERS += \
    LndTools/AbstractLndProcessManager.hpp \
    LndTools/LndBackupManager.hpp \
    LndTools/LndGrpcClient.hpp \
    LndTools/LndProcessManager.hpp \
    LndTools/LndTypes.hpp \
    LndTools/Protos/Common.grpc.pb.h \
    LndTools/Protos/Common.pb.h \
    LndTools/Protos/KeychainService.grpc.pb.h \
    LndTools/Protos/KeychainService.pb.h \
    LndTools/Protos/LightWalletService.grpc.pb.h \
    LndTools/Protos/LightWalletService.pb.h \
    LndTools/Protos/autopilot.grpc.pb.h \
    LndTools/Protos/autopilot.pb.h \
    LndTools/Protos/google/api/annotations.pb.h \
    LndTools/Protos/google/api/http.pb.h \
    LndTools/Protos/google/api/httpbody.pb.h \
    LndTools/Protos/invoices.grpc.pb.h \
    LndTools/Protos/invoices.pb.h \
    LndTools/Protos/router.grpc.pb.h \
    LndTools/Protos/router.pb.h \
    LndTools/Protos/rpc.grpc.pb.h \
    LndTools/Protos/rpc.pb.h

SOURCES += \
    LndTools/AbstractLndProcessManager.cpp \
    LndTools/LndBackupManager.cpp \
    LndTools/LndGrpcClient.cpp \
    LndTools/LndProcessManager.cpp \
    LndTools/LndTypes.cpp \
    LndTools/Protos/Common.grpc.pb.cc \
    LndTools/Protos/Common.pb.cc \
    LndTools/Protos/KeychainService.grpc.pb.cc \
    LndTools/Protos/KeychainService.pb.cc \
    LndTools/Protos/LightWalletService.grpc.pb.cc \
    LndTools/Protos/LightWalletService.pb.cc \
    LndTools/Protos/autopilot.grpc.pb.cc \
    LndTools/Protos/autopilot.pb.cc \
    LndTools/Protos/google/api/annotations.pb.cc \
    LndTools/Protos/google/api/http.pb.cc \
    LndTools/Protos/google/api/httpbody.pb.cc \
    LndTools/Protos/invoices.grpc.pb.cc \
    LndTools/Protos/invoices.pb.cc \
    LndTools/Protos/router.grpc.pb.cc \
    LndTools/Protos/router.pb.cc \
    LndTools/Protos/rpc.grpc.pb.cc \
    LndTools/Protos/rpc.pb.cc
