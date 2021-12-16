INCLUDEPATH += \
    $$PWD

win32 {
    CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../lndtools/release
    CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../lndtools/debug
}


LIBS += \
    -L$$OUT_PWD/../lndtools \
    -llndtools

include($$PWD/../qgrpc/qgrpc.pri)
include($$PWD/../bitcoin/libbitcoin.pri)
include($$PWD/../common/libcommon.pri)


win32:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../lndtools/release/liblndtools.a
else:win32:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../lndtools/debug/liblndtools.a
else: PRE_TARGETDEPS += $$OUT_PWD/../lndtools/liblndtools.a

