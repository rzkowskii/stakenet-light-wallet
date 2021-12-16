INCLUDEPATH += \
    $$PWD \
    $$PWD/vendor/include

win32 {
    CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../core/release
    CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../core/debug
}

LIBS += \
    -L$$OUT_PWD/../core \
    -lcore


include($$PWD/../bitcoin/libbitcoin.pri)
include($$PWD/../networking/libnetworking.pri)
include($$PWD/../lndtools/lndtools.pri)
include($$PWD/../../modules/qtpromise/qtpromise.pri)
include($$PWD/../../modules/libzmq/libzmq.pri)
include($$PWD/../lightningswaps/lightningswaps.pri)

android {
    include($$PWD/../../modules/libopenssl/libopenssl.pri)
}

LIBS += \
    -lssl \
    -lcrypto

win32:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../core/release/libcore.a
else:win32:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../core/debug/libcore.a
else: PRE_TARGETDEPS += $$OUT_PWD/../core/libcore.a
