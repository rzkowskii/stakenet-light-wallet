#ifndef CHAINVIEWMODEL_HPP
#define CHAINVIEWMODEL_HPP

#include <Tools/Common.hpp>

#include <QObject>
#include <QPointer>
#include <boost/optional.hpp>

class ApplicationViewModel;
class ChainManager;
class AbstractChainManager;
class QTimer;
class ChainView;

class ChainViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(AssetID assetID READ assetID WRITE setAssetID NOTIFY assetIDChanged)
    Q_PROPERTY(unsigned chainHeight READ chainHeight NOTIFY updated)

public:
    explicit ChainViewModel(QObject* parent = nullptr);
    unsigned chainHeight() const;

    AssetID assetID() const;
    void setAssetID(AssetID assetId);

signals:
    void updated();
    void assetIDChanged();

public slots:
    void initialize(ApplicationViewModel* appViewModel);

private slots:
    void updateBlockHeight();

private:
    void initChainView();

private:
    QTimer* _updateTimer = nullptr;
    boost::optional<AssetID> _assetID;
    AbstractChainManager* _chainManager{ nullptr };
    std::shared_ptr<ChainView> _chainView;
    unsigned _chainHeight{ 0 };
};

#endif // CHAINVIEWMODEL_HPP
