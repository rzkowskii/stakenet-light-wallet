#ifndef LNDPROCESSMANAGER_HPP
#define LNDPROCESSMANAGER_HPP

#include <QCoreApplication>
#include <QDir>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>

#include <LndTools/AbstractLndProcessManager.hpp>
#include <LndTools/LndGrpcClient.hpp>
#include <LndTools/LndTypes.hpp>

class LndGrpcClient;

class LndProcessManager : public AbstractLndProcessManager {
    Q_OBJECT
public:
    explicit LndProcessManager(LndGrpcClient* client, const DaemonConfig& daemonCfg,
        QDir rootLndDir, int grpcPort, const AssetLndConfig& lndData, QObject* parent = nullptr);

    ~LndProcessManager() override;

    void start() override;
    void stop() override;
    QStringList getNodeConf() const override;
    QStringList getNodeWatchTowers() const override;

private:
    QPointer<LndGrpcClient> _client;
    DaemonConfig _daemonCfg;
    AssetLndConfig _assetLndData;
    QDir _rootLndDir;
    int _grpcPort;
    QDir _binaryDir;
};

#endif // LNDPROCESSMANAGER_HPP
