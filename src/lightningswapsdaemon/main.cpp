#include <CrashReporting/CrashReporting.hpp>
#include <LssdConfig.hpp>
#include <LssdSwapClientFactory.hpp>
#include <Networking/NetworkConnectionState.hpp>
#include <QCoreApplication>
#include <SwapGRPCServer.hpp>
#include <SwapService.hpp>
#include <Utils/Logging.hpp>

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDir>
#include <QLoggingCategory>
#include <QStandardPaths>

static const QString SENTRY_DSN("https://e4f9d55566b74d01a89d7993af6461cb@sentry.io/5187798");

#ifdef Q_OS_WIN

google::protobuf::MessageLite::MessageLite(class google::protobuf::MessageLite const&) {}

google::protobuf::Message::Message(class google::protobuf::Message const&) {}

#endif

//==============================================================================

class LssdApplication : public QCoreApplication {

    // QCoreApplication interface
public:
    static QCommandLineParser parser;
    static const QCommandLineOption portOpt;
    static const QCommandLineOption orderbookUrlOpt;
    static const QCommandLineOption versionOpt;
    static const QCommandLineOption dataDirPath;
    static const QCommandLineOption orderbookAPISecretOpt;

    LssdApplication(int& argc, char** argv)
        : QCoreApplication(argc, argv)
    {
        _reporter = CrashReporting::Create(GetCrashReportingDir().absolutePath(), SENTRY_DSN);
#ifdef QT_NO_DEBUG
        _reporter->start("Production", LSSD_GIT_SHA1_BUILD);
#endif
        qSetMessagePattern("%{time yyyy/MM/dd h:mm:ss} | %{category} | %{threadid} | %{message}");
        setApplicationVersion(LssdVersion());
        qInstallMessageHandler(MessageHandler);

        LogDebug() << "Swap daemon starting"
                   << QString("%1(%2)").arg(LssdVersion()).arg(LssdNumericVersion())
                   << "commit hash:" << LSSD_GIT_SHA1_BUILD;

        QLoggingCategory::setFilterRules("stakenet.swaps.debug=true\n"
                                         "stakenet.grpc.debug=false\n"
                                         "stakenet.orderbook.debug=true");

        ProcessArguments(argc, argv);
        DumpArugments();
    }

    ~LssdApplication() { _reporter->stop(); }

    bool notify(QObject* recv, QEvent* event) override
    {
        try {
            return QCoreApplication::notify(recv, event);
        } catch (std::exception& ex) {
            LogCritical() << "Terminating LSSD application due to exception:" << ex.what();
            std::rethrow_exception(std::current_exception());
        } catch (...) {
            LogCritical() << "Terminating LSSD application due to unknown exception";
            // breakpad will catch this as "Unhandled C++ Exception", so we have at least minidump
            std::rethrow_exception(std::current_exception());
        }
    }

    static QDir GetCrashReportingDir()
    {
        QDir dir(GetDataDir().absoluteFilePath("reports"));
        if (!dir.exists()) {
            dir.mkpath(".");
        }
        return dir;
    }

    static QDir GetDataDir()
    {
        QDir dir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation));
        auto innerPath = "Stakenet/lssd";

        if (!dir.exists(innerPath)) {
            dir.mkpath(innerPath);
        }
        dir.cd(innerPath);

        return dir;
    }

    static void MessageHandler(
        QtMsgType type, const QMessageLogContext& context, const QString& msg)
    {
        auto logMessage = qFormatLogMessage(type, context, msg);

        QString prefix("../../../xsn-wallet/src/");
        logMessage = logMessage.remove(prefix);

        static QDir dir = GetDataDir();

        if (!dir.exists())
            dir.mkpath(".");

        QFile logFile(dir.absoluteFilePath("lssd.log"));

        if (logFile.size() > 50 * 1024 * 1024) {
            dir.remove(dir.absoluteFilePath("lssd_old.log"));

            logFile.copy(dir.absoluteFilePath("lssd_old.log"));
            logFile.resize(0);
        }

        if (logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            QTextStream stream(&logFile);
            stream << logMessage << Qt::endl;
        }
    }

    static void ProcessArguments(int& argc, char** argv)
    {
        QStringList args;
        for (int i = 0; i < argc; ++i) {
            args << QString(argv[i]);
        }

        parser.addHelpOption();
        parser.addOptions({ portOpt, orderbookUrlOpt, versionOpt, orderbookAPISecretOpt, dataDirPath });

        parser.process(args);
    }

    static bool IsArgSet(QCommandLineOption option) { return parser.isSet(option); }

    static QString GetArg(QCommandLineOption option) { return parser.value(option); }

    static void DumpArugments()
    {
        LogCDebug(General) << "Running app with args:";
        for (auto&& option : parser.optionNames()) {
            auto value = CanPrintArg(option) ? parser.value(option) : QString{};
            LogCDebug(General) << "\t" << option << parser.value(option);
        }
    }

private:
    static bool CanPrintArg(QString option)
    {
        static const std::set<QString> blacklist{ orderbookAPISecretOpt.names().front() };

        return blacklist.count(option) == 0;
    }

private:
    CrashReporting::Ptr _reporter;
};

//==============================================================================

QCommandLineParser LssdApplication::parser;
const QCommandLineOption LssdApplication::portOpt("port", "Number of port", "Port number");
const QCommandLineOption LssdApplication::orderbookUrlOpt(
    "orderbookUrl", "Web socket url to connect to the orderbook", "orderbook-url");
const QCommandLineOption LssdApplication::versionOpt({ "v", "version" }, "Show version");
const QCommandLineOption LssdApplication::orderbookAPISecretOpt(
    "orderbookAPISecret", "Api secret that is obtained from Stakenet DEX", "secret");
const QCommandLineOption LssdApplication::dataDirPath (
    "dataDirPath", "path to store all data", "data dir path");

//==============================================================================

int main(int argc, char* argv[])
{
    LssdApplication a(argc, argv);

    if (LssdApplication::IsArgSet(LssdApplication::versionOpt)) {
        std::cout << LssdNumericVersion();
        return 0;
    }

    QString port = LssdApplication::IsArgSet(LssdApplication::portOpt)
        ? LssdApplication::GetArg(LssdApplication::portOpt)
        : QString("50051");

    QThread swapServiceWorker;
    swapServiceWorker.start();

    NetworkConnectionState::Initialize();

    swaps::SwapService::Config cfg;
    cfg.dataDirPath = LssdApplication::IsArgSet(LssdApplication::dataDirPath)
            ? LssdApplication::GetArg(LssdApplication::dataDirPath)
            : LssdApplication::GetDataDir().absolutePath();
    cfg.orderbookUrl = LssdApplication::IsArgSet(LssdApplication::orderbookUrlOpt)
        ? LssdApplication::GetArg(LssdApplication::orderbookUrlOpt)
        : decltype(cfg)::DefaultOrderbookUrl();

    const bool apiSecretSet = LssdApplication::IsArgSet(LssdApplication::orderbookAPISecretOpt);

    cfg.botMakerSecret = apiSecretSet
        ? LssdApplication::GetArg(LssdApplication::orderbookAPISecretOpt)
        : QString{};

    cfg.payFee = !apiSecretSet;

    auto lndClientFactory = std::make_unique<LssdSwapClientFactory>();
    cfg.swapClientFactory = lndClientFactory.get();

    swaps::SwapService service(cfg);
    service.moveToThread(&swapServiceWorker);
    SwapGRPCServer swapServer(service, *lndClientFactory, port.toInt());

    QMetaObject::invokeMethod(&service, [&service] {
        LogDebug() << "Swap service starting";
        service.start();
    });
    QMetaObject::invokeMethod(&a,
        [&swapServer, port] {
            LogDebug() << "Swap server running on port" << port;
            std::cout << "Swap server running on port: " << port.toStdString() << std::endl;
            swapServer.run();
            LogDebug() << "Swap server stopped";
        },
        Qt::QueuedConnection);

    a.exec();

    QMetaObject::invokeMethod(
        &service, [&service] { service.stop(); }, Qt::BlockingQueuedConnection);
    LogDebug() << "Swap service stoped";
    swapServiceWorker.quit();
    swapServiceWorker.wait();

    LogDebug() << "SwapServiceWorker stopped";

    return 0;
}
