TEMPLATE = app
CONFIG += console c++14 websockets
CONFIG -= app_bundle
CONFIG += thread
QT += network testlib
TARGET = core_tests

include($$PWD/../lightningswaps/lightningswaps.pri)
include($$PWD/../../vendor/googletest/googletest.pri)
include($$PWD/../core/libcore.pri)

#copydata.commands = $(COPY_DIR) $$PWD/../app/assets/assets_conf.json $$OUT_PWD && $(COPY_DIR) $$PWD/testdata $$OUT_PWD
#first.depends = $(first) copydata

export(first.depends)
#export(copydata.commands)

LIBS += -lz


QMAKE_EXTRA_TARGETS += first copydata

HEADERS += \
    tst_addressmanager.hpp \
    tst_coretests.hpp \
    tst_ethapi.hpp \
    tst_keystorage.hpp \
    tst_portalhttpclient.hpp \
    tst_chain.hpp \
    tst_raiden.hpp \
    tst_regtest.hpp \
    tst_swaps.hpp

SOURCES += \
    main.cpp \
    tst_swaps.cpp

DISTFILES += \
    testdata/chain_data_1.json \
    testdata/tx_data_1.json

