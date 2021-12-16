INCLUDEPATH += \
    $$PWD \
    $$PWD/vendor/include

macx {
    LIBS += -L/usr/local/opt/openssl/lib
}

win32 {
    CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../bitcoin/release
    CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../bitcoin/debug
}

LIBS += \
    -L$$OUT_PWD/../bitcoin \
    -lbitcoin

include($$PWD/../../modules/libsecp256k1/libsecp256k1.pri)
include($$PWD/../../modules/boost/libboost.pri)
include($$PWD/../../modules/leveldb/libleveldb.pri)
include($$PWD/../../modules/libbdb/libbdb.pri)

android {
    include($$PWD/../../modules/libopenssl/libopenssl.pri)
    include($$PWD/../../modules/libgmp/libgmp.pri)
}

LIBS += \
    -ldb_cxx \
    -ldb \
    -lcrypto \
    -lssl


linux|win32|macx {
LIBS += -lgmp
}

win32 {
LIBS += \
    -lWs2_32
}

win32:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../bitcoin/release/libbitcoin.a
else:win32:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../bitcoin/debug/libbitcoin.a
else: PRE_TARGETDEPS += $$OUT_PWD/../bitcoin/libbitcoin.a
