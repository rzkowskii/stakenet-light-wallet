#ifndef ABSTRACTSWAPCLIENTPOOL_HPP
#define ABSTRACTSWAPCLIENTPOOL_HPP

#include <QObject>
#include <QtPromise>
#include <Swaps/Types.hpp>
#include <memory>

namespace swaps {

class AbstractSwapClient;
class AbstractSwapClientFactory;

//==============================================================================

class AbstractSwapClientPool : public QObject {
    Q_OBJECT
public:
    explicit AbstractSwapClientPool(
        AbstractSwapClientFactory& clientFactory, QObject* parent = nullptr);
    ~AbstractSwapClientPool();

    virtual AbstractSwapClient* getClient(std::string currency) const = 0;
    virtual bool hasClient(std::string currency) const = 0;
    virtual std::vector<std::string> activeClients() const = 0;

signals:
    void htlcAccepted(std::string rHash, int64_t amount, std::string currency, QVariantMap payload);
    void resolveRequestReady(ResolveRequest request);
    void resolvedTransferResponseReady(ResolvedTransferResponse response);
    void clientAdded(std::string currency);

public slots:

protected:
    AbstractSwapClientFactory& _clientFactory;
};

//==============================================================================

class AbstractSwapLndClient;

struct AbstractLndSwapClientPool {
    virtual AbstractSwapLndClient* getLndClient(std::string currency) const = 0;
    virtual bool hasLndClient(std::string currency) const = 0;
    virtual ~AbstractLndSwapClientPool();
};

//==============================================================================
}

#endif // ABSTRACTSWAPCLIENTPOOL_HPP
