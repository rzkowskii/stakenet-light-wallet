INCLUDEPATH += \
    $$PWD

win32 {
    CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../tor/release
    CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../tor/debug
}

LIBS += \
        -L$$OUT_PWD/../tor \
        -ltor



win32 {
    LIBS += -lIphlpapi
}

win32:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../tor/release/libtor.a
else:win32:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../tor/debug/libtor.a
else: PRE_TARGETDEPS += $$OUT_PWD/../tor/libtor.a

include($$PWD/../../modules/libtor/libtor.pri)
include($$PWD/../../modules/boost/libboost.pri)
