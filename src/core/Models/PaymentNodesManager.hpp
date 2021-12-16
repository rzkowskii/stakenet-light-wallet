#ifndef PAYMENTNODESMANAGER_HPP
#define PAYMENTNODESMANAGER_HPP

#include <QObject>
#include <Tools/Common.hpp>
#include <memory>

namespace Utils {
class WorkerThread;
}
class WalletAssetsModel;
class ConnextDaemonsManager;
class PaymentNodeInterface;
class LnDaemonsManager;
class AccountDataSource;
class AssetsTransactionsCache;
class AbstractNetworkingFactory;

class PaymentNodesManager : public QObject {
    Q_OBJECT
public:
    explicit PaymentNodesManager(QPointer<Utils::WorkerThread> workerThread,
        QPointer<AssetsTransactionsCache> txCache, QPointer<WalletAssetsModel> assetsModel,
        AccountDataSource &dataSource, AbstractNetworkingFactory &apiClientFactory, QObject* parent = nullptr);
    ~PaymentNodesManager();

    PaymentNodeInterface* interfaceById(AssetID assetID) const;
    QPointer<LnDaemonsManager> lnDaemonManager() const;
    QPointer<ConnextDaemonsManager> connextDaemonManager() const;

private:
    void init();

private:
    std::unique_ptr<LnDaemonsManager> _lnDaemonManager;
    std::unique_ptr<ConnextDaemonsManager> _connextDaemonsManager;
};

#endif // PAYMENTNODESMANAGER_HPP
