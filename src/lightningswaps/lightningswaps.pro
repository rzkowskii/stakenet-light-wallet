#-------------------------------------------------
#
# Project created by QtCreator 2019-09-17T20:44:33
#
#-------------------------------------------------

QT       += websockets
QT       -= gui

TARGET = lightningswaps
TEMPLATE = lib
CONFIG += staticlib c++14

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

include($$PWD/../bitcoin/libbitcoin.pri)
include($$PWD/../lndtools/lndtools.pri)
include($$PWD/../../modules/httpserver/httpserver.pri)

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    Orderbook/AbstractOrderbookClient.cpp \
    Orderbook/OrderbookApiClient.cpp \
    Orderbook/OrderbookClient.cpp \
    Orderbook/OrderbookConnection.cpp \
    Orderbook/OrderbookEventDispatcher.cpp \
    Orderbook/OrderbookSwapPeerPool.cpp \
    Orderbook/Protos/stakenet/orderbook/api.pb.cc \
    Orderbook/Protos/stakenet/orderbook/commands.pb.cc \
    Orderbook/Protos/stakenet/orderbook/events.pb.cc \
    Orderbook/Protos/stakenet/orderbook/models.pb.cc \
    Orderbook/TradingOrdersModel.cpp \
    Orderbook/OrderbookSwapPeer.cpp \
    SwapService.cpp \
    Swaps/AbstractSwapRepository.cpp \
    Swaps/LndSwapClient.cpp \
    Swaps/RaidenHttpResolveService.cpp \
    Swaps/SwapClientPool.cpp \
    Swaps/SwapManager.cpp \
    Swaps/SwapPeerPool.cpp \
    Swaps/SwapRepository.cpp \
    Swaps/Types.cpp \
    Swaps/AbstractSwapPeer.cpp \
    Swaps/Protos/Packets.pb.cc \
    Swaps/AbstractSwapClient.cpp \
    Swaps/AbstractSwapClientPool.cpp \
    Swaps/AbstractSwapPeerPool.cpp \
    Swaps/Packets.cpp \
    Orderbook/Types.cpp \
    Swaps/RaidenSwapClient.cpp

HEADERS += \
    Orderbook/AbstractOrderbookClient.hpp \
    Orderbook/OrderbookApiClient.hpp \
    Orderbook/OrderbookClient.hpp \
    Orderbook/OrderbookConnection.hpp \
    Orderbook/OrderbookEventDispatcher.hpp \
    Orderbook/OrderbookSwapPeerPool.hpp \
    Orderbook/Protos/stakenet/orderbook/api.pb.h \
    Orderbook/Protos/stakenet/orderbook/commands.pb.h \
    Orderbook/Protos/stakenet/orderbook/events.pb.h \
    Orderbook/Protos/stakenet/orderbook/models.pb.h \
    Orderbook/TradingOrdersModel.hpp \
    Orderbook/OrderbookSwapPeer.hpp \
    SwapService.hpp \
    Swaps/AbstractSwapRepository.hpp \
    Swaps/LndSwapClient.hpp \
    Swaps/SwapClientPool.hpp \
    Swaps/SwapManager.hpp \
    Swaps/SwapPeerPool.hpp \
    Swaps/SwapRepository.hpp \
    Swaps/Types.hpp \
    Swaps/AbstractSwapPeer.hpp \
    Swaps/Protos/Packets.pb.h \
    Swaps/AbstractSwapClient.hpp \
    Swaps/Packets.hpp \
    Swaps/AbstractSwapClientPool.hpp \
    Swaps/AbstractSwapPeerPool.hpp \
    Orderbook/Types.hpp \
    Swaps/RaidenSwapClient.hpp \
    Swaps/RaidenHttpResolveService.hpp

unix {
    target.path = /usr/lib
    INSTALLS += target
}

DISTFILES += \
    Orderbook/Protos/protos.proto \
    Orderbook/Protos/stakenet/orderbook/api.proto \
    Orderbook/Protos/stakenet/orderbook/commands.proto \
    Orderbook/Protos/stakenet/orderbook/events.proto \
    Orderbook/Protos/stakenet/orderbook/models.proto \
    Swaps/Protos/Packets.proto \
    lightningswaps.pri
