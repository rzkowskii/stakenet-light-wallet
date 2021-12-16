INCLUDEPATH += \
    $$PWD

win32 {
    CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../common/release
    CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../common/debug
}

include($$PWD/../../modules/qtpromise/qtpromise.pri)

LIBS += \
        -L$$OUT_PWD/../common \
        -lcommon

win32:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../common/release/libcommon.a
else:win32:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../common/debug/libcommon.a
else: PRE_TARGETDEPS += $$OUT_PWD/../common/libcommon.a
