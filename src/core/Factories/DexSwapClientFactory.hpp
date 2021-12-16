#ifndef LNDSWAPCLIENTSFACTORY_HPP
#define LNDSWAPCLIENTSFACTORY_HPP

#include <Swaps/AbstractSwapClientFactory.hpp>

#include <QObject>

class WalletAssetsModel;
class PaymentNodesManager;
class AssetsTransactionsCache;

class DexSwapClientFactory : public QObject, public swaps::AbstractSwapClientFactory {
    Q_OBJECT
public:
    DexSwapClientFactory(AssetsTransactionsCache& txCache, PaymentNodesManager& paymentNodesManager,
        const WalletAssetsModel& assetsModel, QObject* parent = nullptr);

    std::unique_ptr<swaps::AbstractSwapLndClient> createLndSwapClient(
        std::string currency) override;

    std::unique_ptr<swaps::AbstractSwapConnextClient> createConnextSwapClient(
        std::string currency) override;

    std::unique_ptr<swaps::AbstractConnextResolveService> createConnextResolveService() override;

private:
    AssetsTransactionsCache& _txCache;
    PaymentNodesManager& _paymentNodesManager;
    const WalletAssetsModel& _assetsModel;
};

#endif // LNDSWAPCLIENTSFACTORY_HPP
