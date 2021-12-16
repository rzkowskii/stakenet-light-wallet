#ifndef ABSTRACTLNDMANAGER_HPP
#define ABSTRACTLNDMANAGER_HPP

#include <QDir>
#include <QObject>
#include <QPointer>
#include <QVector>
#include <functional>
#include <memory>

#include <LndTools/LndTypes.hpp>
#include <Utils/Utils.hpp>
#include <Data/CoinAsset.hpp>
#include <Models/PaymentNodeInterface.hpp>

class LndGrpcClient;
class AbstractLndProcessManager;
template <class T> class StreamingReadContext;

namespace lnrpc {
class Invoice;
}

class LnDaemonInterface : public PaymentNodeInterface {
    Q_OBJECT
public:
    struct Cfg {
        explicit Cfg(DaemonConfig cfg, QString rootDataDir, int port)
            : daemonConfig(cfg)
            , rootDataDir(rootDataDir)
            , grpcPort(port)
        {
        }

        DaemonConfig daemonConfig;
        QString rootDataDir;
        int grpcPort;
    };

    explicit LnDaemonInterface(
        Cfg config, AssetLndData lndData, std::unique_ptr<LndGrpcClient> client, QObject* parent = nullptr);
    ~LnDaemonInterface() override;

    struct LnBalance {
        int64_t active{ 0 };
        int64_t inactive{ 0 };
        int64_t pending{ 0 };
        int64_t closing{ 0 };

        int64_t allLocal{ 0 };
        int64_t allRemote{ 0 };

        int64_t availableActive{ 0 };
        int64_t availableInactive{ 0 };
        int64_t availablePending{ 0 };

        int64_t allActive{ 0 };
        int64_t allInactive{ 0 };
        int64_t allPending{ 0 };
        int64_t allClosing{ 0 };

        int64_t total() const { return active + inactive + pending; }

        int64_t availableTotal() const
        {
            return availableActive + availableInactive + availablePending;
        }

        bool operator==(const LnBalance& rhs) const
        {
            return memcmp(&active, &rhs.active, sizeof(rhs)) == 0;
        }

        bool operator!=(const LnBalance& rhs) const { return !(*this == rhs); }
    };

    struct ChainSynced {
        bool chain{ false };
        bool graph{ false };
    };

    AbstractPaymentNodeProcessManager* processManager() const override;

    Promise<QString> identifier() const override;
    Promise<void> closeAllChannels(unsigned feeSatsPerByte) override;
    Promise<bool> isChannelsOpened() const override;
    void refreshChannels() override;

    Promise<ChainSynced> syncedToChain() const;
    Promise<LnBalance> balance() const;

    Promise<std::vector<LndChannel>> channels() const;
    Promise<std::vector<LndChannel>> inactiveChannels() const;
    Promise<std::vector<LndChannel>> pendingChannels() const;
    Promise<std::vector<LndChannel>> pendingWaitingCloseChannels() const;
    Promise<std::vector<LndChannel>> pendingForceClosingChannels() const;
    Promise<int> peersNumber() const;

    Promise<QString> addChannelRequest(
        QString identityKey, QString localAmountStr, unsigned feeSatsPerByte);
    Promise<void> closeChannel(
        QString channelOutpoint, bool isChannelInactive, unsigned feeSatsPerByte);
    Promise<LightningPayRequest> decodePayRequest(QString payRequest);
    Promise<bool> verifyHubRoute(int64_t satoshisAmount, bool selfIsSource);
    Promise<void> restoreChannelBackups(std::string multiChanBackup);

    Promise<std::string> invoicePaymentRequestByPreImage(std::string rHash);


    LndGrpcClient* grpcClient() const;

signals:
    void balanceChanged(LnBalance newBalance);
    void hasActiveChannelChanged(bool value);
    void channelsChanged(
        std::vector<LndChannel> newActiveChannels, std::vector<LndChannel> newInactiveChannels);
    void pendingChannelsChanged(std::vector<LndChannel> newChannels);
    void pendingWaitingCloseChannelsChanged(std::vector<LndChannel> newChannels);
    void pendingForceClosingChannelsChanged(std::vector<LndChannel> newChannels);
    void pubKeyChanged(QString newPubKey);
    void numberOfPeersChanged(int newPeersCount);
    void chainSyncedChanged(bool chainSynced, bool graphSynced);
    void hasPendingChannelChanged(bool value);

protected:
    bool event(QEvent* e) override;

private slots:
    void fetchLndInfo();
    Promise<void> lndAddNewConnection(QString identityKey);
    Promise<void> lndAddNewWatchTower(QString identityKey);
    Promise<QString> openChannel(QString pubkey, int64_t localAmount, unsigned feeSatsPerByte);
    void lndGetListChannels();
    void onRunningChanged();
    void tryConnectingToPeers();
    void tryAddingWatchtowers();
    void doHouseKeeping();

private:
    Promise<void> closeChannelByFundingTxid(
        QStringList channelPointInfo, bool isChannelInactive, unsigned feeSatsPerByte);

    void subscribeInvoices();
    void lndGetActiveChannels();
    void lndGetPendingChannels();
    void setResolverCrashed();
    void setResolverRunning();
    void setConnectedToHub(bool value);
    void setTowersConnected(bool value);
    int peersNumberInternal() const;

private:
    qobject_delete_later_unique_ptr<LndGrpcClient> _grpcClient;
    QObject* _executionContext{ nullptr };
    QTimer* _channelsUpdateTimer{ nullptr };
    QTimer* _autoConnectTimer{ nullptr };
    QTimer* _miscUpdateTimer{ nullptr };

    qobject_delete_later_unique_ptr<AbstractLndProcessManager> _processManager;
    // walletbalance
    LnBalance _balance;

    // initial startup
    QDir _lndDir;

    std::vector<LndChannel> _channels;
    std::vector<LndChannel> _inactiveChannels;
    std::vector<LndChannel> _pendingOpenChannels;
    std::vector<LndChannel> _pendingWaitingCloseChannels;
    std::vector<LndChannel> _pendingForceClosingChannels;

    // getinfo
    QString _infoPubKey;
    int _peersNum{ 0 };
    ChainSynced _chainSynced;
    bool _connectedToHub{ false };
    bool _towersConnected{ false };
    bool _invoicesSubscribed{ false };
};

#endif // ABSTRACTLNDMANAGER_HPP
