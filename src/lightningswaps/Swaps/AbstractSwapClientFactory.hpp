#ifndef ABSTRACTSWAPCLIENTFACTORY_HPP
#define ABSTRACTSWAPCLIENTFACTORY_HPP

#include <memory>
#include <string>

namespace swaps {

//==============================================================================

class AbstractSwapLndClient;
class AbstractSwapConnextClient;
class AbstractConnextResolveService;

//==============================================================================

class AbstractSwapClientFactory {
public:
    virtual std::unique_ptr<AbstractSwapLndClient> createLndSwapClient(std::string currency) = 0;
    virtual std::unique_ptr<AbstractSwapConnextClient> createConnextSwapClient(std::string currency)
        = 0;
    virtual std::unique_ptr<AbstractConnextResolveService> createConnextResolveService() = 0;
    virtual ~AbstractSwapClientFactory();
};

//==============================================================================
}

#endif // ABSTRACTSWAPCLIENTFACTORY_HPP
