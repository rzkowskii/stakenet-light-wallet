#ifndef GRPCSERVER_HPP
#define GRPCSERVER_HPP

#include <QObject>
#include <Utils/Utils.hpp>
#include <memory>

class QThread;
class AbstractChainManager;
class AbstractChainDataSource;
class ZMQChainNotifier;
class WalletDataSource;
class UTXOSetDataSource;
class AssetsTransactionsCache;
class WalletAssetsModel;

class GRPCServer : public QObject {
    Q_OBJECT
public:
    explicit GRPCServer(const WalletAssetsModel& assetsModel, AbstractChainManager& chain,
        AbstractChainDataSource& dataSource, AssetsTransactionsCache& txCache,
        WalletDataSource& walletDataSource, UTXOSetDataSource* utxoDataSource,
        QObject* parent = nullptr);
    ~GRPCServer();

    bool isRunning() const;

    void start();
    void stop();

signals:

public slots:

private:
    struct ServerImpl;
    using ServerPtr = std::unique_ptr<ServerImpl>;
    ServerPtr _server;
    Utils::WorkerThread _serverThread;
};

#endif // GRPCSERVER_HPP
