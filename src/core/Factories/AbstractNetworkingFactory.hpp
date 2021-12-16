#ifndef ABSTRACTNETWORKINGFACTORY_HPP
#define ABSTRACTNETWORKINGFACTORY_HPP

#include <QObject>
#include <Tools/Common.hpp>
#include <Utils/Utils.hpp>
#include <memory>

class AbstractBlockExplorerHttpClient;
class AbstractAccountExplorerHttpClient;
class AbstractRemotePriceProvider;
class AbstractWeb3Client;

class AbstractNetworkingFactory : public QObject {
    Q_OBJECT
public:
    explicit AbstractNetworkingFactory(QObject* parent = nullptr);

    using BlockExplorerHttpClient
        = qobject_delete_later_unique_ptr<AbstractBlockExplorerHttpClient>;
    using RemotePriceProviderPtr = qobject_delete_later_unique_ptr<AbstractRemotePriceProvider>;
    using AbstractWeb3ClientPtr = qobject_delete_later_unique_ptr<AbstractWeb3Client>;
    using AccountExplorerHttpClientPtr
        = qobject_delete_later_unique_ptr<AbstractAccountExplorerHttpClient>;

    virtual BlockExplorerHttpClient createBlockExplorerClient(AssetID assetID) = 0;
    virtual RemotePriceProviderPtr createRemotePriceProvider() = 0;
    virtual AbstractWeb3ClientPtr createWeb3Client(uint32_t chainId) = 0;
    virtual AccountExplorerHttpClientPtr createAccountExplorerClient(AssetID assetID) = 0;

    virtual ~AbstractNetworkingFactory();
};

#endif // ABSTRACTNETWORKINGFACTORY_HPP
