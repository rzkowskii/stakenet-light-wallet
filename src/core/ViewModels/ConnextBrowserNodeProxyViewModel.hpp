#ifndef CONNEXTBROWSERNODEPROXYVIEWMODEL_HPP
#define CONNEXTBROWSERNODEPROXYVIEWMODEL_HPP

#include <LndTools/ConnextBrowserNodeApi.hpp>
#include <LndTools/ConnextProcessManager.hpp>
#include <QObject>

class ApplicationViewModel;

class ConnextBrowserNodeProxyViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(ConnextBrowserNodeApiTransport* transport READ transport NOTIFY updated)
    Q_PROPERTY(
        ConnextProcessManager* processManager READ processManager NOTIFY updated)
public:
    explicit ConnextBrowserNodeProxyViewModel(QObject* parent = nullptr);
    ConnextBrowserNodeApiTransport* transport() const;
    ConnextProcessManager* processManager() const;

public slots:
    void initialize(ApplicationViewModel* appViewModel);

signals:
    void updated();

private:
    ConnextBrowserNodeApiTransport* _transport{ nullptr };
    ConnextProcessManager* _processManager{ nullptr };
};

#endif // CONNEXTBROWSERNODEPROXYVIEWMODEL_HPP
