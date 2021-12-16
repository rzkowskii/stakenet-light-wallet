#ifndef PAYMENTNODESTATEMODEL_HPP
#define PAYMENTNODESTATEMODEL_HPP

#include <QObject>

#include <Tools/Common.hpp>

class PaymentNodeStateModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isPendingChannel READ isPendingChannels NOTIFY channelsPendingChanged)
    Q_PROPERTY(QString identifier READ identifier NOTIFY identifierChanged)
    Q_PROPERTY(double nodeBalance READ nodeBalance NOTIFY nodeBalanceChanged)
    Q_PROPERTY(bool isChannelOpened READ isChannelOpened NOTIFY channelsOpenedChanged)
    Q_PROPERTY(Enums::PaymentNodeType nodeType READ nodeType NOTIFY nodeTypeChanged)
    Q_PROPERTY(QVariantMap nodeLocalRemoteBalances READ nodeLocalRemoteBalances NOTIFY nodeAllBalancesChanged)

public:
    explicit PaymentNodeStateModel(AssetID assetID, QObject* parent = nullptr);
    virtual ~PaymentNodeStateModel();

    virtual bool isPendingChannels() const = 0;
    virtual QString identifier() const = 0;
    virtual double nodeBalance() const = 0;
    virtual bool isChannelOpened() const = 0;
    virtual QVariantMap nodeLocalRemoteBalances() const = 0;
    virtual Enums::PaymentNodeType nodeType() const;

signals:
    void nodeBalanceChanged();
    void channelsOpenedChanged();
    void identifierChanged();
    void channelsPendingChanged();
    void nodeTypeChanged();
    void nodeAllBalancesChanged();

protected:
    AssetID _assetID;
    Enums::PaymentNodeType _paymentNodeType;
};

#endif // PAYMENTNODESTATEMODEL_HPP
