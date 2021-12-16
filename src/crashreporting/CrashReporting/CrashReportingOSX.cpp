#include "CrashReporting.hpp"
#include <sentry.h>

//==============================================================================

class CrashReportingImpl : public CrashReporting {
public:
    CrashReportingImpl(QString crashReportingDir, QString dsn)
        : _crashReportingDir(crashReportingDir)
        , _dsn(dsn)
    {
    }

    virtual void start(QString environment, QString release) override final
    {
        sentry_options_t* options = sentry_options_new();
        sentry_options_set_dsn(options, _dsn.toStdString().data());

        if (!environment.isEmpty()) {
            sentry_options_set_environment(options, environment.toStdString().data());
        }

        if (!release.isEmpty()) {
            sentry_options_set_release(options, release.toStdString().data());
        }

        sentry_options_set_database_path(options, _crashReportingDir.toStdString().data());

        sentry_init(options);
    }

	void setClientIdentity(QString identity) override final
	{
	}

    virtual void stop() override final { sentry_shutdown(); }

    QString _crashReportingDir;
    QString _dsn;
};

//==============================================================================

CrashReporting::~CrashReporting() {}

//==============================================================================

CrashReporting::Ptr CrashReporting::Create(QString crashReportingDir, QString dsn)
{
    return CrashReporting::Ptr(new CrashReportingImpl(crashReportingDir, dsn));
}

//==============================================================================
