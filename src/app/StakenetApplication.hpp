#ifndef STAKENETAPPLICATION_HPP
#define STAKENETAPPLICATION_HPP

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QGuiApplication>

#include <CrashReporting/CrashReporting.hpp>

class StakenetApplication : public QGuiApplication {
    Q_OBJECT
public:
    explicit StakenetApplication(int& argc, char** argv);
    ~StakenetApplication();
    // QCoreApplication interface
public:
    bool notify(QObject* recv, QEvent* event) override;

    static void ProcessArguments(int& argc, char** argv);
    static void DumpArugments();
    static bool IsArgSet(QCommandLineOption option);
    static QCommandLineOption forceMobileOpt;
    static QCommandLineOption forceDesktopOpt;
    static QCommandLineOption configOpt;
    static QCommandLineOption environmentOpt;
    static QCommandLineOption debugOpt;
    static QCommandLineOption versionOpt;
    static QCommandLineOption clearWebEngineCacheOpt;
    static QCommandLineParser parser;

    CrashReporting::Ptr _reporter;
};

#endif // STAKENETAPPLICATION_HPP
