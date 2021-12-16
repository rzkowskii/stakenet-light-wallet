#ifndef ABSTRACTCONNEXTAPI_HPP
#define ABSTRACTCONNEXTAPI_HPP

#include <QObject>
#include <QVariantMap>

#include <Utils/Utils.hpp>

//==============================================================================

struct ConnextApiException : std::runtime_error {
    explicit ConnextApiException(QVariantMap error);

    QVariant rawError;
    QString code;
    QString name;
    QString msg;
    QString validationError;
};

//==============================================================================

class AbstractConnextApi : public QObject {
    Q_OBJECT
public:
    explicit AbstractConnextApi(QObject* parent = nullptr);

    struct DepositTxParams {
        int32_t assetID{ 0 };
        int32_t chainId{ 0 };
        QString tokenAddress;
        QString channelAddress;
        QString amount; // denominated in wei
        QString publicIdentifier;
    };

    virtual Promise<QString> getPublicIdentifier() = 0;
    virtual Promise<std::vector<QString>> getChannelsAddresses(QString publicIdentifier) = 0;
    virtual Promise<QVariantMap> getChannel(QString publicIdentifier, QString channelAddress) = 0;
    virtual Promise<QByteArray> transferResolve(QVariantMap payload) = 0;
    virtual Promise<QVector<QVariantMap>> getChannelsList(QString publicIdentifier) = 0;
    virtual Promise<QString> transferCreate(QVariantMap payload) = 0;
    virtual Promise<QString> setupChannel(QVariantMap payload) = 0;
    virtual Promise<void> sendDeposit(DepositTxParams payload) = 0;
    virtual Promise<QString> reconcile(QVariantMap payload) = 0;
    virtual Promise<void> withdraw(QVariantMap payload) = 0;
    virtual Promise<void> initNode(QString mnemonic) = 0;
    virtual Promise<QString> restoreState(QVariantMap payload) = 0;
    virtual Promise<QVector<QVariantMap>> getTransfers(QVariantMap payload) = 0;

    virtual bool isActive() = 0;

signals:
    void connected();
};

//==============================================================================

#endif // ABSTRACTCONNEXTAPI_HPP
