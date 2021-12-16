#ifndef CONNEXTPROCESSMANAGER_HPP
#define CONNEXTPROCESSMANAGER_HPP

#include <QObject>
#include <QPointer>
#include <QtPromise>

#include <LndTools/AbstractPaymentNodeProcessManager.hpp>
#include <LndTools/LndTypes.hpp>

class DockerApiClient;

class ConnextProcessManager : public AbstractPaymentNodeProcessManager {
    Q_OBJECT
public:
    template <class T> using Promise = QtPromise::QPromise<T>;

    explicit ConnextProcessManager(ConnextDaemonConfig& daemonCfg, QObject* parent = nullptr);

    void start() override;
    void stop() override;
    QStringList getNodeConf() const override;

public slots:
    void onBrowserNodeReady(bool ready);

signals:
    void requestStart();
    void requestStop();

private:
    ConnextDaemonConfig _daemonCfg;
};

#endif // CONNEXTPROCESSMANAGER_HPP
