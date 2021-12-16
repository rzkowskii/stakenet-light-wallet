#ifndef APICLIENTNETWORKINGFACTORY_HPP
#define APICLIENTNETWORKINGFACTORY_HPP

#include <Factories/AbstractNetworkingFactory.hpp>
#include <QObject>

class QNetworkAccessManager;

class ApiClientNetworkingFactory : public AbstractNetworkingFactory {
    Q_OBJECT
public:
    explicit ApiClientNetworkingFactory(QObject* parent = nullptr);
    ~ApiClientNetworkingFactory() override;

    BlockExplorerHttpClient createBlockExplorerClient(AssetID assetID) override;
    RemotePriceProviderPtr createRemotePriceProvider() override;
    AbstractWeb3ClientPtr createWeb3Client(uint32_t chainId) override;
    AccountExplorerHttpClientPtr createAccountExplorerClient(AssetID assetID) override;

private:
    QNetworkAccessManager* _networkAccessManager{ nullptr };
};

#endif // APICLIENTNETWORKINGFACTORY_HPP
