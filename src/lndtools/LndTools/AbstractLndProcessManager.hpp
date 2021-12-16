#ifndef ABSTRACTLNDPROCESSMANAGER_HPP
#define ABSTRACTLNDPROCESSMANAGER_HPP

#include <QObject>
#include <QProcess>

#include <LndTools/AbstractPaymentNodeProcessManager.hpp>

class AbstractLndProcessManager : public AbstractPaymentNodeProcessManager {
public:
    explicit AbstractLndProcessManager(QObject* parent = nullptr);
    virtual ~AbstractLndProcessManager();

    virtual QStringList getNodeWatchTowers() const = 0;

protected:
    QProcess _lndProcess;
};

#endif // ABSTRACTLNDPROCESSMANAGER_HPP
