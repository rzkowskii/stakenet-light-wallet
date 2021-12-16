#ifndef ETHFIXEDGASPROVIDER_HPP
#define ETHFIXEDGASPROVIDER_HPP

#include <EthCore/AbstractEthGasProvider.hpp>
#include <QObject>

class EthFixedGasProvider : public AbstractEthGasProvider {
    Q_OBJECT
public:
    explicit EthFixedGasProvider(GasPrices prices, QObject* parent = nullptr);

    Promise<GasPrices> fetchGasPrice() override;

private:
    GasPrices _prices;
};

#endif // ETHFIXEDGASPROVIDER_HPP
