#ifndef ABSTRACTETHGASPROVIDER_HPP
#define ABSTRACTETHGASPROVIDER_HPP

#include <QObject>
#include <EthCore/Types.hpp>
#include <Utils/Utils.hpp>

class AbstractEthGasProvider : public QObject {
    Q_OBJECT
public:
    explicit AbstractEthGasProvider(QObject* parent = nullptr);

    // Has gas price that is used all over the network,
    // all values are in wei
    struct GasPrices {
        eth::u256 slow;
        eth::u256 standard;
        eth::u256 fast;

        QString toString() const;
    };

    virtual Promise<GasPrices> fetchGasPrice() = 0;
};

#endif // ABSTRACTETHGASPROVIDER_HPP
