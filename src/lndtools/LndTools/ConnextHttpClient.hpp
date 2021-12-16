#ifndef CONNEXTHTTPCLIENT_HPP
#define CONNEXTHTTPCLIENT_HPP

#include <LndTools/AbstractConnextApi.hpp>
#include <Networking/RequestHandlerImpl.hpp>
#include <memory>

class ConnextHttpClient : public AbstractConnextApi {
    Q_OBJECT
public:
    explicit ConnextHttpClient(
        std::unique_ptr<RequestHandlerImpl>&& requestHandler, QObject* parent = nullptr);
    Promise<QString> getPublicIdentifier() override;
    Promise<std::vector<QString>> getChannelsAddresses(QString publicIdentifier) override;
    Promise<QVariantMap> getChannel(QString publicIdentifier, QString channelAddress) override;
    Promise<QByteArray> transferResolve(QVariantMap payload) override;
    Promise<QVector<QVariantMap>> getChannelsList(QString publicIdentifier) override;
    Promise<QString> transferCreate(QVariantMap payload) override;
    Promise<QString> setupChannel(QVariantMap payload) override;
    Promise<void> sendDeposit(DepositTxParams payload) override;
    Promise<QString> reconcile(QVariantMap payload) override;
    Promise<void> withdraw(QVariantMap payload) override;
    Promise<void> initNode(QString mnemonic) override;
    Promise<QString> restoreState(QVariantMap payload) override;
    Promise<QVector<QVariantMap>> getTransfers(QVariantMap payload) override;

    bool isActive() override;

signals:

private:
    RequestHandlerImpl* _requestHandler{ nullptr };
};

#endif // CONNEXTHTTPCLIENT_HPP
