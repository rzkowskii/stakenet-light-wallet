#ifndef CONNEXTDAEMONINTERFACE_HPP
#define CONNEXTDAEMONINTERFACE_HPP

#include <Data/CoinAsset.hpp>
#include <LndTools/ConnextTypes.hpp>
#include <Models/PaymentNodeInterface.hpp>
#include <Utils/Utils.hpp>

#include <QObject>
#include <memory>

class AbstractConnextApi;
class ConnextProcessManager;
class WalletAssetsModel;

class ConnextDaemonInterface : public PaymentNodeInterface {
    Q_OBJECT
public:
    explicit ConnextDaemonInterface(AbstractConnextApi* connextClient,
        ConnextProcessManager* processManager, const WalletAssetsModel& assetsModel,
        AssetID assetID, QObject* parent = nullptr);
    ~ConnextDaemonInterface() override;

    struct ConnextBalance {
        eth::u256 localBalance;
        eth::u256 remoteBalance;

        bool operator==(const ConnextBalance& rhs) const
        {
            return localBalance == rhs.localBalance && remoteBalance == rhs.remoteBalance;
        }

        bool operator!=(const ConnextBalance& rhs) const { return !(*this == rhs); }
    };

    Promise<std::vector<ConnextChannel>> channels() const;
    Promise<QString> identifier() const override;
    Promise<void> closeAllChannels(unsigned feeSatsPerByte) override;
    AbstractPaymentNodeProcessManager* processManager() const override;

    Promise<bool> isChannelsOpened() const override;
    void refreshChannels() override;

    Promise<QString> openChannel(QString identityKey, int64_t amount, AssetID assetID) const;
    AbstractConnextApi* httpClient() const;
    Promise<QString> depositChannel(int64_t amount, QString channelAddress, AssetID assetID) const;
    Promise<void> withdraw(
        QString recipientAddress, int64_t amount, QString channelAddress, AssetID assetID) const;
    Promise<bool> verifyHubRoute(int64_t satoshisAmount, bool selfIsSource);
    Promise<ConnextBalance> channelsBalance() const;
    void setChannelsBalance(const ConnextBalance& newBalance);
    void setNodeInitialized(bool value);
    Promise<QString> reconcileChannel(QString channelAddress, AssetID assetID) const;
    Promise<QString> restoreChannel(AssetID assetID) const;

signals:
    void identifierChanged(QString identifier);
    void channelsChanged(std::vector<ConnextChannel> newActiveChannels);
    void hasChannelChanged(bool value);
    void channelsBalanceChanged(ConnextBalance balance);

private:
    void init();
    void fetchInfo();
    Promise<bool> isActive() const;
    void connextGetActiveChannels();
    Promise<QString> deposit(QString channelAddress, eth::u256 amount, AssetID assetID) const;
    Promise<QString> reconcile(QString channelAddress, QString contractAddress) const;
    void refreshPayments();

private slots:
    void connextGetListChannels();

private:
    QObject* _executionContext{ nullptr };
    QPointer<AbstractConnextApi> _connextClient;
    std::vector<ConnextChannel> _channels;
    QPointer<ConnextProcessManager> _processManager;
    QString _identifier;
    QString _tokenAddress;
    AssetID _assetID;

    QTimer* _channelsUpdateTimer{ nullptr };
    const WalletAssetsModel& _assetsModel;

    bool _nodeInitialized{ false };
    ConnextBalance _channelsBalance;
};
Q_DECLARE_METATYPE(eth::u256);

#endif // CONNEXTDAEMONINTERFACE_HPP
