#ifndef ETHGASSTATIONHTTPCLIENT_HPP
#define ETHGASSTATIONHTTPCLIENT_HPP

#include <EthCore/AbstractEthGasProvider.hpp>
#include <QObject>

class QNetworkAccessManager;

class EthGasStationHttpClient : public AbstractEthGasProvider {
    Q_OBJECT
public:
    explicit EthGasStationHttpClient(
        QNetworkAccessManager* accessManager, QObject* parent = nullptr);

    Promise<GasPrices> fetchGasPrice() override;

private:
    QPointer<QNetworkAccessManager> _accessManager;
};

#endif // ETHGASSTATIONHTTPCLIENT_HPP
