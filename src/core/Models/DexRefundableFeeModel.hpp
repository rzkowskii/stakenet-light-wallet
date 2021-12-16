#ifndef DEXREFUNDABLEFEEMODEL_HPP
#define DEXREFUNDABLEFEEMODEL_HPP

#include <QObject>
#include <QPointer>
#include <boost/optional.hpp>

#include <Tools/Common.hpp>
#include <Utils/Utils.hpp>

namespace swaps {
class RefundableFeeManagerState;
}

class ApplicationViewModel;
class WalletAssetsModel;

class DexRefundableFeeModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(unsigned currentAssetID READ currentAssetID WRITE setCurrentAssetID NOTIFY
            currentAssetIDChanged)
    Q_PROPERTY(double refudanbleAmount READ refudanbleAmount NOTIFY refudanbleAmountChanged)
public:
    explicit DexRefundableFeeModel(QObject* parent = nullptr);

    AssetID currentAssetID() const;
    void setCurrentAssetID(AssetID assetID);

    double refudanbleAmount() const;

public slots:
    void initialize(ApplicationViewModel* applicationViewModel);

signals:
    void currentAssetIDChanged();
    void refudanbleAmountChanged();

private slots:
    void onDexServiceReady();
    void onAssetIDUpdated();

private:
    void setRefundableAmount(Balance amount);
    Promise<std::string> attemtOurCurrency() const;

private:
    QPointer<ApplicationViewModel> _appViewModel;
    QPointer<WalletAssetsModel> _assetsModel;
    QPointer<swaps::RefundableFeeManagerState> _feeRefunding;
    boost::optional<AssetID> _currentAssetID;
    Balance _refundableAmount{ 0 };
};

#endif // DEXREFUNDABLEFEEMODEL_HPP
