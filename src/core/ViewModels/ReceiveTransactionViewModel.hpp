#ifndef RECEIVETRANSACTIONVIEWMODEL_HPP
#define RECEIVETRANSACTIONVIEWMODEL_HPP

#include <Data/CoinAsset.hpp>
#include <QObject>

class ReceiveTransactionViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isUTXOType READ isUTXOType CONSTANT)
public:
    explicit ReceiveTransactionViewModel(CoinAsset::Type assetType, bool isLightning, QObject* parent = nullptr);
    ~ReceiveTransactionViewModel();

    CoinAsset::Type assetType() const;
    bool isLightning() const;
    bool isUTXOType() const;

public slots:
    virtual void requestAllKnownAddressesById() = 0;

signals:
    void allKnownAddressByIdGenerated(QStringList addresses);

private:
    CoinAsset::Type _assetType{ CoinAsset::Type::Invalid };
    bool _isLightning{ false };
};

#endif // RECEIVETRANSACTIONVIEWMODEL_HPP
