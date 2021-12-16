INCLUDEPATH += \
    $$PWD \

win32 {
    CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../qgrpc/release
    CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../qgrpc/debug
}

LIBS += \
    -L$$OUT_PWD/../qgrpc \
    -lqgrpc

include($$PWD/../common/libcommon.pri)
include($$PWD/../bitcoin/libbitcoin.pri)
include($$PWD/../../modules/grpc/grpc.pri)
