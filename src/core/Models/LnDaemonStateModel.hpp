#ifndef LNDSTATUSMODEL_HPP
#define LNDSTATUSMODEL_HPP

#include <QObject>
#include <Tools/Common.hpp>

#include <Models/LnDaemonInterface.hpp>
#include <Models/PaymentNodeStateModel.hpp>

class AutopilotModel;
class AssetsBalance;

class LnDaemonStateModel : public PaymentNodeStateModel {
    Q_OBJECT
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")

    Q_PROPERTY(QVariantMap lndAllBalances READ lndAllBalances NOTIFY lndAllBalancesChanged)
    Q_PROPERTY(bool autopilotActive READ isAutopilotActive NOTIFY autopilotActiveChanged)
    Q_PROPERTY(AutopilotStatus autopilotStatus READ autopilotStatus NOTIFY autopilotStatusChanged)
    Q_PROPERTY(QVariantMap autopilotDetails READ autopilotDetails NOTIFY autopilotDetailsChanged)
    Q_PROPERTY(LndStatus nodeStatus READ nodeStatus NOTIFY nodeStatusChanged)
    Q_PROPERTY(QString nodeStatusString READ nodeStatusString NOTIFY nodeStatusChanged)
    Q_PROPERTY(QString nodeStatusColor READ nodeStatusColor NOTIFY nodeStatusChanged)

public:
    enum class AutopilotStatus {
        AutopilotDisabled,
        AutopilotChainSyncing,
        AutopilotGraphSyncing,
        AutopilotNoPeers,
        AutopilotNotEnoughCoins,
        AutopilotActive,
        AutopilotNone,
    };
    Q_ENUM(AutopilotStatus)

    enum class LndStatus {
        LndNotRunning,
        LndChainSyncing,
        LndGraphSyncing,
        WaitingForPeers,
        LndActive,
        LndNone
    };
    Q_ENUM(LndStatus)

    explicit LnDaemonStateModel(AssetID assetID, LnDaemonInterface* daemonInterface,
        AutopilotModel* autopilotModel, const AssetsBalance* assetsBalance,
        QObject* parent = nullptr);
    ~LnDaemonStateModel() override;

    bool isPendingChannels() const override;
    QString identifier() const override;
    double nodeBalance() const override;
    bool isChannelOpened() const override;
    Enums::PaymentNodeType nodeType() const override;
    QVariantMap nodeLocalRemoteBalances() const override;

    LndStatus nodeStatus() const;
    QString nodeStatusString() const;
    QString nodeStatusColor() const;

    bool isLndRunning() const;
    int numPeers() const;
    QVariantMap lndAllBalances() const;
    AutopilotStatus autopilotStatus() const;
    bool isAutopilotActive() const;
    QVariantMap autopilotDetails() const;

signals:
    void lndAllBalancesChanged();
    void numPeersChanged();
    void autopilotStatusChanged();
    void autopilotActiveChanged();
    void autopilotDetailsChanged();
    void nodeStatusChanged();

private slots:
    void onUpdateAutopilotStatus();
    void onUpdateLndStatus();

private:
    void init();
    void setPubKey(QString pubKey);
    void setPeersNumber(int peersNum);
    void setBalance(LnDaemonInterface::LnBalance balance);
    void setHasActiveChannels(bool value);
    void setIsSynced(bool value);
    void setAutopilotStatus(AutopilotStatus status);
    void setLndStatus(LndStatus status);
    void setChainSynced(bool syncedToChain, bool syncedToGraph);
    void setHasPendingChannels(bool value);

private:
    QString _pubKey;
    QPointer<LnDaemonInterface> _lndManager;
    QPointer<AutopilotModel> _autopilotModel;
    QPointer<const AssetsBalance> _balance;
    LnDaemonInterface::LnBalance _allBalance;
    AutopilotStatus _autopilotStatus;
    LndStatus _lndStatus;
    int _numPeers{ 0 };
    bool _hasActiveChannels{ false };
    bool _syncedToChain{ false };
    bool _syncedToGraph{ false };
    bool _hasPendingChannels{ false };
};

#endif // LNDSTATUSMODEL_HPP
