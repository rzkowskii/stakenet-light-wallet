#ifndef OPENCHANNELMODEL_HPP
#define OPENCHANNELMODEL_HPP

#include <QObject>

#include <Tools/Common.hpp>

class WalletAssetsModel;

/*!
 * \brief The OpenChannelModel class is an interface for opening L2 channels
 * Class that derivce from this interface implement logic for opening channels
 * for LND or Connext networks. All communication happens in event driven way using
 * signals for reporting success/failure and state changes.
 */
class OpenChannelModel : public QObject {
    Q_OBJECT
public:
    explicit OpenChannelModel(
        AssetID assetID, WalletAssetsModel* walletAssetsModel, QObject* parent = nullptr);
    AssetID assetID() const;
    virtual void createOpenChannelRequest(
        QString destination, QString localAmount, qint64 networkFee)
        = 0;
    virtual void confirmRequest() = 0;
    virtual void cancelRequest() = 0;

signals:
    void requestCreated(QString destination, qint64 networkFee, qint64 deployFee);
    void requestCreatingFailed(QString errorMessage);
    void channelOpened();
    void channelOpeningFailed(QString errorMessage);

protected:
    QPointer<WalletAssetsModel> _walletAssetsModel;

private:
    AssetID _assetID;
};

#endif // OPENCHANNELMODEL_HPP
