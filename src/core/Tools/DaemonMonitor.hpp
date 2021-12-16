#ifndef DAEMONMONITOR_HPP
#define DAEMONMONITOR_HPP

#include <QObject>
#include <QPointer>
#include <Utils/Utils.hpp>
#include <functional>

class QTimer;
class AbstractPaymentNodeProcessManager;

class DaemonMonitor : public QObject {
    Q_OBJECT
public:
    explicit DaemonMonitor(AbstractPaymentNodeProcessManager* processManager,
        std::function<bool()> canStart, QObject* parent = nullptr);
    ~DaemonMonitor();

    Promise<void> start();
    Promise<void> stop();

private slots:
    void onCheckState();

private:
    QPointer<AbstractPaymentNodeProcessManager> _processManager;
    QPointer<QTimer> _stopTimer;
    QTimer* _restartTimer{ nullptr };
    std::function<bool()> _canStart;
};

#endif // DAEMONMONITOR_HPP
