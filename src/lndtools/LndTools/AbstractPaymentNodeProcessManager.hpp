#ifndef ABSTRACTPAYMENTNODEPROCESSMANAGER_HPP
#define ABSTRACTPAYMENTNODEPROCESSMANAGER_HPP

#include <QObject>
#include <QStringList>

class AbstractPaymentNodeProcessManager : public QObject {
    Q_OBJECT
public:
    explicit AbstractPaymentNodeProcessManager(QObject* parent = nullptr);

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual QStringList getNodeConf() const = 0;
    bool running() const;

signals:
    void runningChanged();

protected:
    void setRunning(bool running);

private:
    bool _running{ false };
};

#endif // ABSTRACTPAYMENTNODEPROCESSMANAGER_HPP
