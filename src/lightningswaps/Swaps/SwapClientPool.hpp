#ifndef SWAPCLIENTPOOL_HPP
#define SWAPCLIENTPOOL_HPP

#include <QPointer>
#include <SwapService.hpp>
#include <Swaps/AbstractSwapClientPool.hpp>
#include <Utils/Utils.hpp>

class LndGrpcClient;

namespace swaps {

class AbstractSwapLndClient;
class AbstractSwapConnextClient;
class AbstractConnextResolveService;
struct ConnextConfig;

class SwapClientPool : public AbstractSwapClientPool, public AbstractLndSwapClientPool {
    Q_OBJECT
public:
    explicit SwapClientPool(AbstractSwapClientFactory& clientFactory, QObject* parent = nullptr);
    ~SwapClientPool() override;

    AbstractSwapClient* getClient(std::string currency) const override;
    AbstractSwapLndClient* getLndClient(std::string currency) const override;
    bool hasClient(std::string currency) const override;
    bool hasLndClient(std::string currency) const override;
    std::vector<std::string> activeClients() const override;

    void addLndClient(std::string currency);
    void addConnextClient(std::string currency);

private:
    std::map<std::string, std::unique_ptr<AbstractSwapLndClient>> _lndClients;
    std::map<std::string, std::unique_ptr<AbstractSwapConnextClient>> _connextClients;
    std::unique_ptr<AbstractConnextResolveService> _connextResolver;
};
}

#endif // SWAPCLIENTPOOL_HPP
