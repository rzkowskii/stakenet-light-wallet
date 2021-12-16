#ifndef ABSTRACTCHAINSYNCMANAGERFACTORY_HPP
#define ABSTRACTCHAINSYNCMANAGERFACTORY_HPP

#include <QObject>
#include <QPointer>
#include <Tools/Common.hpp>
#include <Utils/Utils.hpp>
#include <memory>

class AbstractChainSyncManager;
class WalletAssetsModel;
class AbstractNetworkingFactory;
class Chain;

using AbstractChainSyncManagerPtr = qobject_delete_later_unique_ptr<AbstractChainSyncManager>;

class AbstractChainSyncManagerFactory : public QObject {
    Q_OBJECT
public:
    explicit AbstractChainSyncManagerFactory(QPointer<WalletAssetsModel> assetsModel,
        QPointer<AbstractNetworkingFactory> networkingFactory, QObject* parent = nullptr);

    virtual ~AbstractChainSyncManagerFactory();

    virtual AbstractChainSyncManagerPtr createAPISyncManager(Chain& chain) = 0;
    virtual AbstractChainSyncManagerPtr createRescanSyncManager(Chain& chain) = 0;

signals:

public slots:

protected:
    const WalletAssetsModel* assetsModel() const;
    AbstractNetworkingFactory* networkingFactory() const;

private:
    QPointer<WalletAssetsModel> _assetsModel;
    QPointer<AbstractNetworkingFactory> _networkingFactory;
};

#endif // ABSTRACTCHAINSYNCMANAGERFACTORY_HPP
