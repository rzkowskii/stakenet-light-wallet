#ifndef EXCHANGEGATEWAYFACTORY_HPP
#define EXCHANGEGATEWAYFACTORY_HPP

#include <memory>

namespace tradingbot::gateway {

class ExchangeGateway;

using ExchangeGatewayRef = std::unique_ptr<ExchangeGateway>;

struct ExchangeGatewayFactory {
    virtual ~ExchangeGatewayFactory();
    virtual ExchangeGatewayRef createGateway() = 0;
};
}

#endif // EXCHANGEGATEWAYFACTORY_HPP
