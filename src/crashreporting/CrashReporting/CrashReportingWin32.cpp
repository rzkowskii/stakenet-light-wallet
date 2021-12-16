#include "CrashReporting.hpp"
#include <sentry.h>
#include <mutex>

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
		static std::once_flag once;
		std::call_once(once, [identity = identity.toStdString()] {
			sentry_value_t user = sentry_value_new_object();
			sentry_value_set_by_key(user, "id", sentry_value_new_string(identity.data()));
			sentry_set_user(user);
		});
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
