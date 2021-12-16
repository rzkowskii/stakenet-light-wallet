#ifndef CONNEXTSTATEMODEL_HPP
#define CONNEXTSTATEMODEL_HPP

#include <QObject>
#include <Tools/Common.hpp>

#include <Models/ConnextDaemonInterface.hpp>
#include <Models/PaymentNodeStateModel.hpp>

class ConnextStateModel : public PaymentNodeStateModel {
    Q_OBJECT
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")
    Q_PROPERTY(ConnextStatus nodeStatus READ nodeStatus NOTIFY nodeStatusChanged)
    Q_PROPERTY(QString nodeStatusString READ nodeStatusString NOTIFY nodeStatusChanged)
    Q_PROPERTY(QString nodeStatusColor READ nodeStatusColor NOTIFY nodeStatusChanged)


public:
    enum class ConnextStatus { ConnextSyncing, ConnextNotRunning, ConnextActive, ConnextNone };
    Q_ENUM(ConnextStatus)

    explicit ConnextStateModel(
        AssetID assetID, ConnextDaemonInterface* daemonInterface, QObject* parent = nullptr);
    ~ConnextStateModel() override;

    ConnextStatus nodeStatus() const;
    QString nodeStatusString() const;
    QString nodeStatusColor() const;

    bool isPendingChannels() const override;
    QString identifier() const override;
    double nodeBalance() const override;
    bool isChannelOpened() const override;
    Enums::PaymentNodeType nodeType() const override;
    QVariantMap nodeLocalRemoteBalances() const override;

signals:
    void nodeStatusChanged();

private slots:
    void onUpdateConnextStatus();

private:
    void init();
    void setIdentifier(QString identifier);
    void setConnextStatus(ConnextStatus status);
    void setHasChannel(bool value);
    void setBalance(ConnextDaemonInterface::ConnextBalance balance);

private:
    QString _identifier;
    ConnextStatus _connextStatus;
    QPointer<ConnextDaemonInterface> _connextManager;
    bool _hasChannel{ false };
    ConnextDaemonInterface::ConnextBalance _channelsBalance{ 0 };
};

#endif // CONNEXTSTATEMODEL_HPP
