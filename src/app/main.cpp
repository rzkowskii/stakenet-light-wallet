// Copyright (c) 2018 The XSN developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <AppUpdater.hpp>
#include <QGuiApplication>
#include <QMLUtils.hpp>
#include <QQmlApplicationEngine>
#include <QQmlFileSelector>
#include <QSslConfiguration>
#include <QSslSocket>
#include <StakenetApplication.hpp>
#include <StakenetConfig.hpp>
#include <Tools/AppConfig.hpp>
#include <Tools/Common.hpp>
#include <Utils/Logging.hpp>
#include <ViewModels/ApplicationViewModel.hpp>
#include <ViewModels/WalletViewModel.hpp>
#include <Models/WalletDataSource.hpp>

#include <QProcess>
#include <QSettings>
#include <google/protobuf/message.h>
#include <iostream>

#ifdef Q_OS_WIN

google::protobuf::MessageLite::MessageLite(class google::protobuf::MessageLite const&) {}

google::protobuf::Message::Message(class google::protobuf::Message const&) {}

#endif

//==============================================================================

int main(int argc, char* argv[])
{
    StakenetApplication::ProcessArguments(argc, argv);

    if (StakenetApplication::IsArgSet(StakenetApplication::versionOpt)) {
        std::cout << StakenetNumericVersion();
        return 0;
    }

    int result;
    QVariantMap updatePayload;
    {
        StakenetApplication app(argc, argv);

        if (ApplicationViewModel::IsEmulated()) {
            CleanLndDir(true);
        }

        ApplicationViewModel::Instance()->init();
        AppUpdater::Instance(); // init app updater to handle cleanup

		QObject::connect(ApplicationViewModel::Instance()->walletViewModel(), 
			&WalletViewModel::walletLoaded, qApp, [&app] {
			app._reporter->setClientIdentity(ApplicationViewModel::Instance()->dataSource()->identityPubKey());
		});

        StakenetApplication::DumpArugments();

        LogCDebug(General) << "QSslSocket::sslLibraryBuildVersionString"
                           << QSslSocket::sslLibraryBuildVersionString();
        LogCDebug(General) << "QSslSocket::sslLibraryVersionString"
                           << QSslSocket::sslLibraryVersionString();
        LogCDebug(General) << "QSslConfiguration::defaultConfiguration"
                           << QSslConfiguration::defaultConfiguration().protocol();

        QMLUtils::RegisterQMLTypes();
        RegisterCommonQtTypes();

        QQmlApplicationEngine engine;
        QMLUtils::setContextProperties(engine.rootContext());

        bool isMobile = ApplicationViewModel::IsMobile();

        if (StakenetApplication::IsArgSet(StakenetApplication::forceDesktopOpt)
            && StakenetApplication::IsArgSet(StakenetApplication::forceMobileOpt)) {
            LogCDebug(General) << "Both force-mobile and force-desktop where set. Quitting.";
            return 0;
        }

        if (StakenetApplication::IsArgSet(StakenetApplication::forceMobileOpt)) {
            isMobile = true;
        }

        if (StakenetApplication::IsArgSet(StakenetApplication::forceDesktopOpt)) {
            isMobile = false;
        }

        engine.addImportPath("qrc:/");
        engine.rootContext()->setContextProperty("isMobile", isMobile);

        QQmlFileSelector* selector = new QQmlFileSelector(&engine);

        if (isMobile) {
            selector->setExtraSelectors({ "android" });
        }

        engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
        if (engine.rootObjects().isEmpty())
            return -1;

        QObject::connect(&app, &QCoreApplication::aboutToQuit,
            [] { ApplicationViewModel::Instance()->destroyContext(); });

        result = app.exec();

        updatePayload = app.property("pending_update").toMap();
    }

    LogCDebug(General) << "---------------------  Closing app  --------------------------";

    if (!updatePayload.isEmpty()) {
        QString program = updatePayload.value("program").toString();
        QStringList arguments = updatePayload.value("args").toStringList();

        LogCDebug(General) << "Starting updater:" << program << arguments;

        if (!QProcess::startDetached(program, arguments)) {
            LogCDebug(General) << "Failed to start process" << program << "with args" << arguments;
        }
    }

    return result;
}
