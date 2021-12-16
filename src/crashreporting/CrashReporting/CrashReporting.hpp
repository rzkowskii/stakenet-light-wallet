#ifndef CRASHREPORTING_HPP
#define CRASHREPORTING_HPP

#include <QString>
#include <memory>

class CrashReporting {
public:
    using Ptr = std::unique_ptr<CrashReporting>;
    virtual ~CrashReporting();

    virtual void start(QString environment, QString release) = 0;
    virtual void stop() = 0;

	virtual void setClientIdentity(QString identity) = 0;

    static Ptr Create(QString crashReportingDir, QString dsn);
};

#endif // CRASHREPORTING_HPP
