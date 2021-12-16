INCLUDEPATH += \
    $$PWD

win32 {
    CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../libupdater/release
    CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../libupdater/debug
}

LIBS += \
        -L$$OUT_PWD/../libupdater \
        -lupdater

win32:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libupdater/release/libupdater.a
else:win32:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libupdater/debug/libupdater.a
else: PRE_TARGETDEPS += $$OUT_PWD/../libupdater/libupdater.a

include($$PWD/../../modules/qtpromise/qtpromise.pri)
include($$PWD/../networking/libnetworking.pri)
