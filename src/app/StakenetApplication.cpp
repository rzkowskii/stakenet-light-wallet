#include "StakenetApplication.hpp"
#include <StakenetConfig.hpp>
#include <Tools/AppConfig.hpp>
#include <Utils/Logging.hpp>
#include <ViewModels/ApplicationViewModel.hpp>

#include <QDir>
#include <QFile>
#include <QIcon>
#include <QQuickWebEngineProfile>
#include <QStandardPaths>
#include <QtWebEngine>
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/thread/thread_time.hpp>

//==============================================================================

QCommandLineParser StakenetApplication::parser;
QCommandLineOption StakenetApplication::forceMobileOpt("force-mobile", "Force mobile UI");
QCommandLineOption StakenetApplication::forceDesktopOpt("force-desktop", "Force desktop UI");
QCommandLineOption StakenetApplication::configOpt("config", "App config", "path");
QCommandLineOption StakenetApplication::environmentOpt(
    "environment", "Staging or production environment", "value");
QCommandLineOption StakenetApplication::debugOpt("debug", "Debug mode");
QCommandLineOption StakenetApplication::versionOpt({ "v", "version" }, "Show version");
QCommandLineOption StakenetApplication::clearWebEngineCacheOpt(
    "clear-webengine-cache", "Clear webengine cache");

static const QString SENTRY_DSN(
    "https://c140a27794174490ace64f2c3abb0b41@o200001.ingest.sentry.io/5186574");

//==============================================================================

static bool ObtainFileLock()
{
    QDir dataLocation = GetDataDir(ApplicationViewModel::IsEmulated());
    QString path = dataLocation.absoluteFilePath(".lock");

    if (!QFile::exists(path)) {
        QFile file(path);
        file.open(QIODevice::WriteOnly);
        file.close();
    }

    try {
        static boost::interprocess::file_lock flock(path.toStdString().c_str());
        if (!flock.timed_lock(boost::get_system_time() + boost::posix_time::seconds(3))) {
            LogCDebug(General) << "Failed to obtain lock:" << path;
            return false;
        }
    } catch (std::exception& ex) {
        LogCDebug(General) << "Failed to acquire lock:" << ex.what();
        return false;
    }

    return true;
}

//==============================================================================

static void MessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    static const int MAX_LOG_LINE_SIZE = 1000;
    auto truncatedMsg = msg.size() > MAX_LOG_LINE_SIZE
        ? msg.left(MAX_LOG_LINE_SIZE).append(QString("(truncated %1)").arg(msg.size()))
        : msg;

    auto logMessage = qFormatLogMessage(type, context, truncatedMsg);

    QString prefix("../../../xsn-wallet/src/");
    logMessage = logMessage.remove(prefix);
#ifdef QT_NO_DEBUG

    static auto isEmulated = ApplicationViewModel::Instance()->isEmulated();
    QDir dir = GetDataDir(isEmulated);

    if (!dir.exists())
        dir.mkpath(".");

    QFile logFile(dir.absoluteFilePath("debug.log"));

    if (logFile.size() > 5 * 1024 * 1024) {
        dir.remove(dir.absoluteFilePath("debug_old.log"));

        logFile.copy(dir.absoluteFilePath("debug_old.log"));
        logFile.resize(0);
    }

    if (logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream stream(&logFile);
        stream << logMessage << Qt::endl;
    }
#else
    QTextStream stream(stderr);
    stream << logMessage << Qt::endl;
#endif
}

//==============================================================================

static bool HandleUserDefinedEnvironment(QString value)
{
    bool isStaging = value == QLatin1String("staging");
    bool isProduction = value == QLatin1String("production");

    if (isStaging) {
        ApplicationViewModel::UseStagingEnv(true);
    } else if (isProduction) {
        ApplicationViewModel::UseStagingEnv(false);
    } else {
        return false;
    }

    return true;
}

//==============================================================================

StakenetApplication::StakenetApplication(int& argc, char** argv)
    : QGuiApplication(argc, argv)
{
    _reporter = CrashReporting::Create(
        GetCrashReportingDir(ApplicationViewModel::IsEmulated()).absolutePath(), SENTRY_DSN);

#ifdef QT_NO_DEBUG
    _reporter->start("Production", STAKENET_GIT_SHA1_BUILD);
#endif

    qSetMessagePattern("%{time yyyy/MM/dd h:mm:ss} | %{category} | %{threadid} | %{message}");
    setAttribute(Qt::AA_EnableHighDpiScaling);

    QtWebEngine::initialize();

    if (IsArgSet(clearWebEngineCacheOpt)) {
        QQuickWebEngineProfile::defaultProfile()->clearHttpCache();
    }

    qInstallMessageHandler(MessageHandler);
    setApplicationName("stakenet-wallet");
    setOrganizationName("Stakenet");
    setOrganizationDomain("stakenet.io");
    setWindowIcon(QIcon(":/images/Stakenet_Icon_128.png"));
    setApplicationVersion(StakenetVersion());

    LogCDebug(General) << "---------------------  Starting app  --------------------------";

    if (parser.isSet(environmentOpt)) {
        if (!HandleUserDefinedEnvironment(parser.value(environmentOpt))) {
            LogCCritical(General) << "Invalid environment, use `staging` or `production`";
            exit(1);
        }
    } else {
        ApplicationViewModel::UseStagingEnv(STAKENET_DEFAULT_STAGING);
    }

    QString configPath;
    if (parser.isSet(configOpt)) {
        configPath = parser.value(configOpt);
    }

    if (!parser.isSet(debugOpt)) {
        QLoggingCategory::setFilterRules("stakenet.sync.debug=false\n"
                                         "stakenet.db.debug=false\n"
                                         "stakenet.api.debug=false\n"
                                         "stakenet.wallet.debug=false\n"
                                         "stakenet.grpc.debug=false\n"
                                         "stakenet.lnd.debug=false\n"
                                         "stakenet.swaps.debug=false\n"
                                         "stakenet.connext.debug=false\n"
                                         "stakenet.orderbook.debug=false");
    }

    try {
        AppConfig::Instance().parse(*ApplicationViewModel::Instance()->assetsModel(), configPath);
    } catch (std::exception& ex) {
        LogCDebug(General) << ex.what();
        exit(0);
    }

    LogCDebug(General) << "Starting app, version:"
                       << QString("%1(%2)").arg(StakenetVersion()).arg(StakenetNumericVersion())
                       << "commit hash:" << QString(STAKENET_GIT_SHA1_BUILD);

    LogCDebug(General) << "Starting app, version:"
                       << QString("%1(%2)").arg(StakenetVersion()).arg(StakenetNumericVersion())
                       << "commit hash:" << QString(STAKENET_GIT_SHA1_BUILD);

    if (!ObtainFileLock()) {
        ::exit(0);
    }
}

//==============================================================================

StakenetApplication::~StakenetApplication()
{
    _reporter->stop();
}

//==============================================================================

bool StakenetApplication::notify(QObject* recv, QEvent* event)
{
    try {
        return QGuiApplication::notify(recv, event);
    } catch (std::exception& ex) {
        LogCritical() << "Terminating Stakenet application due to exception:" << ex.what();
        std::rethrow_exception(std::current_exception());
    } catch (...) {
        LogCritical() << "Terminating Stakenet application due to unknown exception";
        // breakpad will catch this as "Unhandled C++ Exception", so we have at least minidump
        std::rethrow_exception(std::current_exception());
    }
}

//==============================================================================

void StakenetApplication::ProcessArguments(int& argc, char** argv)
{
    QStringList args;
    for (int i = 0; i < argc; ++i) {
        args << QString(argv[i]);
    }

    parser.addOptions({ forceMobileOpt, forceDesktopOpt, configOpt, debugOpt, versionOpt,
        clearWebEngineCacheOpt, environmentOpt });

    parser.process(args);
}

//==============================================================================

void StakenetApplication::DumpArugments()
{
    LogCDebug(General) << "Running app with args:";
    for (auto&& option : parser.optionNames()) {
        LogCDebug(General) << "\t" << option << parser.value(option);
    }
}

//==============================================================================

bool StakenetApplication::IsArgSet(QCommandLineOption option)
{
    return parser.isSet(option);
}

//==============================================================================
