#ifndef LNDAEMONSSTATEMANAGER_HPP
#define LNDAEMONSSTATEMANAGER_HPP

#include <QObject>
#include <Tools/Common.hpp>
#include <Models/PaymentNodeInterface.hpp>

class WalletAssetsModel;
class PaymentNodeStateModel;
class PaymentNodesManager;
class AssetsBalance;

class PaymentNodeStateManager : public QObject {
    Q_OBJECT
public:
    explicit PaymentNodeStateManager(const WalletAssetsModel& assetsModel,
        const AssetsBalance& assetsBalance, const PaymentNodesManager& paymentManager,
        QObject* parent = nullptr);
    ~PaymentNodeStateManager();

    PaymentNodeStateModel* stateModelById(AssetID assetID) const;
    Enums::PaymentNodeType stateModelTypeById(AssetID assetID) const;

signals:

private:
    void init(const WalletAssetsModel& assetsModel);

private:
    const AssetsBalance& _assetsBalance;
    const PaymentNodesManager& _paymentManager;
    std::map<AssetID, std::unique_ptr<PaymentNodeStateModel>> _stateModels;
};

#endif // LNDAEMONSSTATEMANAGER_HPP
