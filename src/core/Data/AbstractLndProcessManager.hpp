#ifndef ABSTRACTLNDPROCESSMANAGER_HPP
#define ABSTRACTLNDPROCESSMANAGER_HPP

#include <QDir>
#include <QObject>
#include <QProcess>
#include <QTimer>
#include <functional>

#include <Tools/AppConfig.hpp>

//==============================================================================

#if 0

class UserInteractionCmd : public QObject
{
    Q_OBJECT
public:
    explicit UserInteractionCmd(QString cmd, QStringList args = {}, QObject *parent = nullptr);

    QString command() const { return _command; }
    QStringList args() const { return _args; }

    using ResponseStreamReader = std::function<QString()>;
    using CommandStreamWriter = std::function<void(QString)>;

    CommandStreamWriter commandStream;

    void notifyResponseReady(QString response);
    void notifyErrorResponseReady(QString response);
    void notifyFinished();

signals:
    void responseReady(QString response);
    void errorResponseReady(QString response);
    void finished();

private slots:
    void onDataRecv();

private:
    QString _command;
    QStringList _args;
    QTimer *_dataRecvTimer { nullptr };
    QString _stdResponseAccum;
    QString _errResponseAccum;
};

//==============================================================================

class AbstractLndProcessManager : public QObject
{
    Q_OBJECT
public:
    explicit AbstractLndProcessManager(QObject *parent = nullptr);
    virtual ~AbstractLndProcessManager();

    using ReplyReady = std::function<void (QString, QString)>;
    virtual void start() = 0;
    virtual void stop() const = 0;
    virtual void executeCLICommand(QString command, ReplyReady onReplyReady, QStringList additionalArgs = {}) const = 0;
    virtual void executeUserInterractionCommand(UserInteractionCmd *cmd) const = 0;
    virtual void executeSwap(QStringList args, ReplyReady onReplyReady) const = 0;
    virtual QStringList getNodeConf() const = 0;
    virtual QStringList getNodeWatchTowers() const = 0;

    bool running() const;

signals:
    void runningChanged();

public slots:

protected:
    void setRunning(bool running);

protected:
    QProcess _lndProcess;

private:
    bool _running { false };

};

#endif

//==============================================================================

#endif // ABSTRACTLNDPROCESSMANAGER_HPP
