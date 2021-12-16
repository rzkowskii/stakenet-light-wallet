QT += websockets

INCLUDEPATH += \
    $$PWD \

win32 {
    CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../lightningswaps/release
    CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../lightningswaps/debug
}

LIBS += \
    -L$$OUT_PWD/../lightningswaps \
    -llightningswaps

include($$PWD/../bitcoin/libbitcoin.pri)
include($$PWD/../lndtools/lndtools.pri)


win32:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../lightningswaps/release/liblightningswaps.a
else:win32:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../lightningswaps/debug/liblightningswaps.a
else: PRE_TARGETDEPS += $$OUT_PWD/../lightningswaps/liblightningswaps.a
