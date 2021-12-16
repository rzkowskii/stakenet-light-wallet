#ifndef ABSTRACTSWAPCONNEXTCLIENT_HPP
#define ABSTRACTSWAPCONNEXTCLIENT_HPP

#include <Swaps/AbstractSwapClient.hpp>

namespace swaps {

//==============================================================================

class AbstractSwapConnextClient : public AbstractSwapClient {
    Q_OBJECT
public:
    AbstractSwapConnextClient();

    virtual ~AbstractSwapConnextClient();

public:
    virtual Promise<std::string> sendPaymentWithPayload(QVariantMap payload) = 0;

public slots:
    virtual void onTransferResolved(ResolvedTransferResponse details) = 0;
};

//==============================================================================

/*!
 * \brief The AbstractConnextResolveService class describes interface for delivering connext
 * specific events during conditional payments.
 */
class AbstractConnextResolveService : public QObject {
    Q_OBJECT
public:
    explicit AbstractConnextResolveService(QObject *parent = nullptr);

signals:
    void receivedResolveRequest(ResolveRequest request);
    void receivedResolvedTransfer(ResolvedTransferResponse response);
};

//==============================================================================

}

#endif // ABSTRACTSWAPCONNEXTCLIENT_HPP
