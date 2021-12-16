#ifndef RPCSERVER_HPP
#define RPCSERVER_HPP

#if 0

#include <QObject>
#include <Tools/Common.hpp>
#include <Utils/Utils.hpp>

namespace jsonrpc {
class AbstractServerConnector;
}

class AbstractChainManager;
class AbstractChainDataSource;
class ZMQChainNotifier;
class WalletDataSource;

class RPCServer : public QObject
{
    Q_OBJECT
public:
    explicit RPCServer(QObject *parent = nullptr);
    ~RPCServer();

    void start();
    void stop();

    void registerChainCalls(AbstractChainManager &chain);
    void registerChainNotifier(AbstractChainDataSource &dataSource);
    void registerWalletCalls(WalletDataSource &walletDataSource);

signals:

public slots:

private:
    struct Impl;
    using ImplPtr = std::unique_ptr<Impl>;
    using ConnectorPtr = std::unique_ptr<jsonrpc::AbstractServerConnector>;

    struct Backend {
        ImplPtr impl;
        ConnectorPtr connector;
        int rpcPort;
        int zmqPort;
        AssetID assetID;
    };

    std::vector<Backend> _rpcBackends;
};

#endif

#endif // RPCSERVER_HPP
