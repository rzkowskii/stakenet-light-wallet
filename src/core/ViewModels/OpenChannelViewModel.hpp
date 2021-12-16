#ifndef OPENCHANNELVIEWMODEL_HPP
#define OPENCHANNELVIEWMODEL_HPP

#include <QObject>
#include <QPointer>
#include <QVariantMap>
#include <boost/optional.hpp>

#include <Tools/Common.hpp>

class ApplicationViewModel;
class OpenChannelModel;

//!
//! \brief The OpenChannelViewModel class is an abstraction over OpenChannelModel
//! which will be used by QML to make open channel requests for opening LND or Connext
//! payment channels. Expected happy-path flow is the following:
//! createOpenChannelRequest()
//! emit requestCreated()
//! confirmRequest()
//! emit channelOpened()
//!
class OpenChannelViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(unsigned currentAssetID READ currentAssetID WRITE setCurrentAssetID NOTIFY
            currentAssetIDChanged)
public:
    explicit OpenChannelViewModel(QObject* parent = nullptr);
    ~OpenChannelViewModel();

    AssetID currentAssetID() const;
    void setCurrentAssetID(int assetID);

signals:
    void requestCreated(QString identityKey, qint64 networkFee, qint64 deployFee);
    void requestCreatingFailed(QString errorMessage);
    void channelOpened();
    void channelOpeningFailed(QString errorMessage);
    void currentAssetIDChanged();

public slots:
    void initialize(ApplicationViewModel* applicationViewModel);
    void createOpenChannelRequest(
        QString identityKey, QString localAmount, unsigned feeSatsPerByte);
    void cancelRequest();
    void confirmRequest();

private:
    void onAssetIDUpdated();
    void initOpenChannelModel();

private:
    QPointer<ApplicationViewModel> _applicationViewModel;
    QPointer<OpenChannelModel> _openChannelModel;
    boost::optional<AssetID> _currentAssetID;
};

#endif // OPENCHANNELVIEWMODEL_HPP
