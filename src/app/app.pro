# allows to add DEPLOYMENTFOLDERS and links to the V-Play library and QtCreator auto-completion

QT += quick concurrent svg websockets webkit

TARGET = Stakenet
DESTDIR = $$OUT_PWD/appdir/usr/bin
CONFIG += c++14
QMAKE_LFLAGS += -fstack-protector

# Add more folders to ship with the application here
RESOURCES += \
    assets/assets.qrc \
    fonts/fonts.qrc \
    qml/qml.qrc

QML_IMPORT_PATH += $$PWD/qml
GIT_CURRENT_HASH = $$system(git rev-parse HEAD)
DEFINES += GIT_CURRENT_COMMIT=\\\"$$GIT_CURRENT_HASH\\\"

# NOTE: for PUBLISHING, perform the following steps:
# 1. comment the DEPLOYMENTFOLDERS += qmlFolder line above, to avoid shipping your qml files with the application (instead they get compiled to the app binary)
# 2. uncomment the resources.qrc file inclusion and add any qml subfolders to the .qrc file; this compiles your qml files and js files to the app binary and protects your source code
# 3. change the setMainQmlFile() call in main.cpp to the one starting with "qrc:/" - this loads the qml files from the resources
# for more details see the "Deployment Guides" in the V-Play Documentation

# during development, use the qmlFolder deployment because you then get shorter compilation times (the qml files do not need to be compiled to the binary but are just copied)
# also, for quickest deployment on Desktop disable the "Shadow Build" option in Projects/Builds - you can then select "Run Without Deployment" from the Build menu in Qt Creator if you only changed QML files; this speeds up application start, because your app is not copied & re-compiled but just re-interpreted


# The .cpp file which was generated for your project. Feel free to hack it.

SOURCES += \
    MouseEventSpy.cpp \
    main.cpp \
    QMLUtils.cpp \
    QMLClipboardAdapter.cpp \
    AppUpdater.cpp \
    TorManager.cpp

win32 {
    SOURCES += CrashReporting/CrashReportingWin32.cpp
}

linux {
    SOURCES += CrashReporting/CrashReportingLinux.cpp
}


!android {
include($$PWD/../tor/tor.pri)
include($$PWD/../../modules/quazip/quazip.pri)
}

include($$PWD/../libupdater/libupdater.pri)
include($$PWD/../core/libcore.pri)
include($$PWD/../../modules/breakpad/libbreakpad.pri)
linux:!android|macx|win32 {
LIBS += -lz
}

win32 {
    LIBS += -lzstd \
            -lIphlpapi
}

RC_ICONS = $$PWD/assets/xsn128.ico
RC_FILE = $$PWD/assets/winResourse.rc

ICON = $$PWD/assets/xsn128.icns

DISTFILES += \
    qml/Components/ActionButton.qml \
    qml/Components/ActionDialog.qml \
    qml/Components/CheckableButton.qml \
    qml/Components/CoinsCombobox.qml \
    qml/Components/ColorOverlayImage.qml \
    qml/Components/ComboBoxItem.qml \
    qml/Components/CopiedAddress.qml \
    qml/Components/CurrencyComboBox.qml \
    qml/Components/CustomizedComboBox.qml \
    qml/Components/IconButton.qml \
    qml/Components/ListHeader.qml \
    qml/Components/MenuIcon.qml \
    qml/Components/MainMenuItem.qml \
    qml/Components/PrimaryButton.qml \
    qml/Components/ProgressDialog.qml \
    qml/Components/RoundedImage.qml \
    qml/Components/SecondaryButton.qml \
    qml/Components/SecondaryLabel.qml \
    qml/Components/TransactionButton.qml \
    qml/Components/TransactionHeader.qml \
    qml/Components/XSNLabel.qml \
    qml/Components/XSNTextArea.qml \
    qml/Pages/EmulatorPage.qml \
    qml/Pages/MainPage.qml \
    qml/Pages/PortfolioPage.qml \
    qml/Pages/SettingsPage.qml \
    qml/Pages/TransactionPage.qml \
    qml/Pages/WalletPage.qml \
    qml/Popups/ReceivePopup.qml \
    qml/Popups/SendPopup.qml \
    qml/Popups/TransactionsDetailsPopup.qml \
    qml/Views/EmulatorView.qml \
    qml/Views/LocalizationView.qml \
    qml/Views/PageHeaderView.qml \
    qml/Views/PortofiloWalletsListView.qml \
    qml/Views/SettingsHeader.qml \
    qml/Views/SettingsView.qml \
    qml/Views/TransactionsListHeaderView.qml \
    qml/Views/TransactionsListView.qml \
    qml/Views/WalletAssetsListView.qml \
    qml/Views/WalletMenuListView.qml \
    qml/Views/WalletPageHeaderView.qml \
    qml/Views/WalletsListHeaderView.qml \
    qml/Views/WalletsListView.qml \
    qml/Views/LightningChannelsListView.qml \
    qml/main.qml

HEADERS += \
    MouseEventSpy.hpp \
    QMLUtils.hpp \
    QMLClipboardAdapter.hpp \
    CrashReporting/CrashReporting.hpp \
    AppUpdater.hpp \
    TorManager.hpp

android {
    ANDROID_EXTRA_LIBS = \
        $$PWD/../../modules/libopenssl/lib/1.0.2p/libssl.so \
        $$PWD/../../modules/libopenssl/lib/1.0.2p/libcrypto.so \
        $$PWD/../../modules/libzmq/lib/libzmq.so
}


