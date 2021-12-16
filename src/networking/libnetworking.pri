INCLUDEPATH += \
    $$PWD \
    $$PWD/vendor/include

win32 {
    CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../networking/release
    CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../networking/debug
}

LIBS += \
        -L$$OUT_PWD/../networking \
        -lnetworking

win32:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../networking/release/libnetworking.a
else:win32:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../networking/debug/libnetworking.a
else: PRE_TARGETDEPS += $$OUT_PWD/../networking/libnetworking.a
