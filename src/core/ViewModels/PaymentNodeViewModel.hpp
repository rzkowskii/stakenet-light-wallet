#ifndef LIGHTNINGVIEWMODEL_HPP
#define LIGHTNINGVIEWMODEL_HPP

#include <QObject>
#include <QPointer>
#include <QStringListModel>
#include <boost/optional.hpp>

#include <Models/PaymentNodeInterface.hpp>
#include <Tools/Common.hpp>

class ApplicationViewModel;
class AssetsBalance;
class AutopilotModel;
class PaymentNodeStateModel;
class PaymentChannelsListModel;

class PaymentNodeViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(unsigned currentAssetID READ currentAssetID WRITE setCurrentAssetID NOTIFY
            currentAssetIDChanged)
    Q_PROPERTY(PaymentNodeStateModel* stateModel READ stateModel NOTIFY stateModelChanged)
    Q_PROPERTY(Enums::PaymentNodeType type READ type NOTIFY stateModelChanged)
    Q_PROPERTY(QStringListModel* hostModel READ hostModel NOTIFY hostModelChanged)
    Q_PROPERTY(
        PaymentChannelsListModel* channelsModel READ channelsModel NOTIFY channelsModelChanged)

public:
    explicit PaymentNodeViewModel(QObject* parent = nullptr);
    ~PaymentNodeViewModel();

    AssetID currentAssetID() const;
    void setCurrentAssetID(int assetID);

    PaymentNodeStateModel* stateModel() const;
    QStringListModel* hostModel() const;
    Enums::PaymentNodeType type() const;
    PaymentChannelsListModel* channelsModel() const;

public slots:
    void initialize(ApplicationViewModel* applicationViewModel);
    bool isLightningAddress(QString address);
    void closeAllChannels(unsigned feeSatsPerByte);
    void closeChannel(QString channelId, bool isChannelInactive, unsigned feeSatsPerByte);
    void activateAutopilot(bool active);
    void changeAutopilotDetails(double allocation, unsigned maxChannels);
    void changeApPreOpenPopupVisibility();
    bool apPreOpenPopupVisibility();
    void invoicePaymentRequestByPreImage(QString rHash);
    void depositChannel(QString amount, QString channelAddress);
    void withdraw(QString recipientAddress, QString amount, QString channelAddress);
    void reconcileChannel(QString channelAddress);
    void restoreChannel();

signals:
    void stateModelChanged();
    void channelsModelChanged();
    void currentAssetIDChanged();
    void channelClosed();
    void channelFailedToClose(QString errMsg);
    void allChannelsClosed();
    void allChannelsFailedToClose(QString errMsg);
    void hostModelChanged();
    void lnInvoiceReady(QString invoice);
    void depositChannelFinished(QString channelAddress);
    void depositChannelFailed(QString errMsg);
    void withdrawFinished();
    void withdrawFailed(QString errMsg);
    void reconcileFinished();
    void reconcileFailed(QString errMsg);
    void restoreChannelFinished();
    void restoreChannelFailed(QString errMsg);

private:
    void onAssetIDUpdated();
    void cancelOrders();
    void initPaymentNode();
    void initChannelsModel();

private:
    ApplicationViewModel* _applicationViewModel{ nullptr };
    boost::optional<AssetID> _currentAssetID;
    QPointer<PaymentNodeInterface> _paymentNodeIn;
    QPointer<PaymentNodeStateModel> _paymentNodeStateModel;
    std::unique_ptr<PaymentChannelsListModel> _paymentChannelsListModel;
    QPointer<QStringListModel> _hostList;
    bool _apPreOpenPopupVisibility{ true };
    Enums::PaymentNodeType _paymentNodeType;
};

#endif // LIGHTNINGVIEWMODEL_HPP
