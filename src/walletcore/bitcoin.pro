#-------------------------------------------------
#
# Project created by QtCreator 2017-08-24T14:24:54
#
#-------------------------------------------------

QT       -= gui

TARGET = bitcoin
TEMPLATE = lib
CONFIG += staticlib c++14

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

INCLUDEPATH += $$PWD/vendor/include

include($$PWD/../../modules/boost/libboost.pri)
include($$PWD/../../modules/leveldb/libleveldb.pri)
include($$PWD/../../modules/libbdb/libbdb.pri)

android {
    include($$PWD/../../modules/libopenssl/libopenssl.pri)
    include($$PWD/../../modules/libgmp/libgmp.pri)
}


# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    crypto/aes.cpp \
    crypto/chacha20.cpp \
    crypto/hmac_sha512.cpp \
    crypto/ripemd160.cpp \
    crypto/sha256.cpp \
    crypto/sha512.cpp \
    script/script.cpp \
    script/standard.cpp \
    support/cleanse.cpp \
    support/lockedpool.cpp \
    base58.cpp \
    bech32.cpp \
    bip39.cpp \
    chainparams.cpp \
    crypter.cpp \
    db.cpp \
    fs.cpp \
    hash.cpp \
    hdchain.cpp \
    key.cpp \
    key_io.cpp \
    keystore.cpp \
    pubkey.cpp \
    random.cpp \
    sync.cpp \
    uint256.cpp \
    utilstrencodings.cpp \
    utiltime.cpp \
    wallet.cpp \
    walletdb.cpp \
    crypto/ctaes/bench.c \
    crypto/ctaes/ctaes.c \
    crypto/ctaes/test.c \
    crypto/aes_helper.c \
    transaction.cpp \
    interfaces.cpp \
    coinselection.cpp \
    fees.cpp \
    policy/feerate.cpp \
    policy/policy.cpp \
    script/sign.cpp \
    script/interpreter.cpp \
    script/script_error.cpp \
    crypto/sha1.cpp \
    golomb/gcs.cpp \
    dbwrapper.cpp \
    txdb.cpp \
    outputtype.cpp \
    arith_uint256.cpp


HEADERS += \
    compat/byteswap.h \
    compat/endian.h \
    crypto/ctaes/ctaes.h \
    crypto/aes.h \
    crypto/chacha20.h \
    crypto/common.h \
    crypto/hmac_sha512.h \
    crypto/ripemd160.h \
    crypto/sha256.h \
    crypto/sha512.h \
    crypto/sph_types.h \
    script/script.h \
    script/standard.h \
    support/allocators/secure.h \
    support/allocators/zeroafterfree.h \
    support/cleanse.h \
    support/events.h \
    support/lockedpool.h \
    base58.h \
    bech32.h \
    bip39.h \
    bip39_english.h \
    chainparams.hpp \
    crypter.h \
    db.h \
    fs.h \
    hash.h \
    hdchain.h \
    key.h \
    key_io.h \
    keystore.h \
    prevector.h \
    pubkey.h \
    random.h \
    secp256k1.h \
    secp256k1_recovery.h \
    serialize.h \
    span.h \
    streams.h \
    sync.h \
    threadsafety.h \
    tinyformat.h \
    uint256.h \
    utilmemory.h \
    utilstrencodings.h \
    utiltime.h \
    wallet.h \
    walletdb.h \
    compat.h \
    transaction.h \
    interfaces.hpp \
    optional.h \
    coinselection.h \
    fees.h \
    policy/feerate.h \
    policy/policy.h \
    script/sign.h \
    script/interpreter.h \
    script/script_error.h \
    crypto/sha1.h \
    golomb/gcs.h \
    golomb/order32.h \
    dbwrapper.h \
    txdb.h \
    outputtype.h \
    arith_uint256.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}

DISTFILES += \
    libbitcoin.pri

macx {
    INCLUDEPATH += "/usr/local/opt/openssl/include"
}
