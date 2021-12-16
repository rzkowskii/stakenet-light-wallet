#include "DexSwapClientFactory.hpp"

#include <Chain/AbstractTransactionsCache.hpp>
#include <Data/WalletAssetsModel.hpp>
#include <LndTools/ConnextBrowserNodeApi.hpp>
#include <LndTools/ConnextHttpClient.hpp>
#include <Models/ConnextDaemonsManager.hpp>
#include <Models/LnDaemonsManager.hpp>
#include <Models/PaymentNodesManager.hpp>
#include <Swaps/AbstractSwapConnextClient.hpp>
#include <Swaps/ConnextSwapClient.hpp>
#include <Swaps/LndSwapClient.hpp>
#include <Tools/LndTools.hpp>

#include <QJSValue>

//==============================================================================

class ConnextBrowserNodeResolverProxy : public swaps::AbstractConnextResolveService {
public:
    ConnextBrowserNodeResolverProxy(
        ConnextBrowserNodeApiTransport* transport, QObject* parent = nullptr)
        : swaps::AbstractConnextResolveService(parent)
    {
        connect(transport, &ConnextBrowserNodeApiTransport::eventConditionalTransferCreated, this,
            [this](
                QVariant payload) { emit receivedResolveRequest(parseResolveRequest(payload)); });
        connect(transport, &ConnextBrowserNodeApiTransport::eventConditionalTransferResolved, this,
            [this](QVariant payload) {
                emit receivedResolvedTransfer(parseResolvedResponse(payload));
            });
    }

    swaps::ResolveRequest parseResolveRequest(QVariant payload)
    {
        swaps::ResolveRequest request;

        auto requestBody = payload.value<QJSValue>().toVariant().toMap().value("transfer").toMap();

        request.rHash = requestBody.value("transferState")
                            .toMap()
                            .value("lockHash")
                            .toString()
                            .mid(2)
                            .toStdString();

        request.amount = static_cast<int64_t>((requestBody.value("balance")
                                                   .toMap()
                                                   .value("amount")
                                                   .toList()
                                                   .first()
                                                   .toString()
                                                   .toDouble()));

        request.expiration = requestBody.value("transferTimeout").toString().toInt();
        request.initiatorIdentifier
            = requestBody.value("initiatorIdentifier").toString().toStdString();
        request.responderIdentifier
            = requestBody.value("responderIdentifier").toString().toStdString();
        request.transferId = requestBody.value("transferId").toString().toStdString();
        request.channelAddress = requestBody.value("channelAddress").toString().toStdString();

        return request;
    }

    swaps::ResolvedTransferResponse parseResolvedResponse(QVariant payload)
    {
        swaps::ResolvedTransferResponse request;
        auto requestBody = payload.value<QJSValue>().toVariant().toMap().value("transfer").toMap();

        request.lockHash = requestBody.value("transferState")
                               .toMap()
                               .value("lockHash")
                               .toString()
                               .mid(2)
                               .toStdString();

        request.rPreimage = requestBody.value("transferResolver")
                                .toMap()
                                .value("preImage")
                                .toString()
                                .mid(2)
                                .toStdString();

        return request;
    }
};

//==============================================================================

class LndSwapClientProxy : public swaps::AbstractSwapLndClient {

    // AbstractSwapClient interface
public:
    LndSwapClientProxy(LndGrpcClient* grpcClient, std::string currency, AssetID assetID,
        AssetsTransactionsCache& txCache)
        : _proxy(grpcClient, currency)
        , _assetID(assetID)
        , _txCache(txCache)
    {
        connect(
            &_proxy, &swaps::LndSwapClient::htlcAccepted, this, &LndSwapClientProxy::htlcAccepted);
    }
    Promise<void> verifyConnection() override { return _proxy.verifyConnection(); }
    Promise<std::string> sendPayment(swaps::SwapDeal deal) override
    {
        return _proxy.sendSwapPayment(deal)
            .tap([this](lnrpc::Payment response) {
                _txCache.cacheById(_assetID).then([assetID = _assetID, response](
                                                      AbstractTransactionsCache* cache) {
                    auto parsed = ParseLnPayment(assetID, response);
                    parsed->tx().set_type(lndtypes::LightningPaymentReason::PAYMENT_SWAP_PAYMENT);
                    cache->addTransactions({ parsed });
                });
            })
            .tapFail([this](lnrpc::Payment failedPayment) {
                _txCache.cacheById(_assetID).then([assetID = _assetID, failedPayment](
                                                      AbstractTransactionsCache* cache) {
                    auto parsed = ParseLnPayment(assetID, failedPayment);
                    parsed->tx().set_type(lndtypes::LightningPaymentReason::PAYMENT_SWAP_PAYMENT);
                    cache->addTransactions({ parsed });
                });

                throw std::runtime_error(PaymentFailureMsg(failedPayment.failure_reason()));
            })
            .then([](lnrpc::Payment payment) { return payment.payment_preimage(); });
    }
    Promise<lnrpc::Payment> sendPayment(
        std::string paymentRequest, lndtypes::LightningPaymentReason reason) override
    {
        return _proxy.sendPayment(paymentRequest, reason)
            .tap([=](lnrpc::Payment response) {
                _txCache.cacheById(_assetID).then(
                    [assetID = _assetID, response, reason](AbstractTransactionsCache* cache) {
                        auto parsed = ParseLnPayment(assetID, response);
                        parsed->tx().set_type(reason);
                        cache->addTransactions({ parsed });
                    });
            })
            .tapFail([this, reason](lnrpc::Payment failedPayment) {
                _txCache.cacheById(_assetID).then(
                    [assetID = _assetID, failedPayment, reason](AbstractTransactionsCache* cache) {
                        auto parsed = ParseLnPayment(assetID, failedPayment);
                        parsed->tx().set_type(reason);
                        cache->addTransactions({ parsed });
                    });

                throw std::runtime_error(PaymentFailureMsg(failedPayment.failure_reason()));
            });
    }
    Promise<std::string> addHodlInvoice(
        std::string rHash, swaps::u256 units, uint32_t cltvExpiry) override
    {
        return _proxy.addHodlInvoice(rHash, units, cltvExpiry).tap([this, rHash] {
            _proxy.lookupInvoice(rHash).then([=](lnrpc::Invoice invoice) {
                _txCache.cacheById(_assetID).then([assetID = _assetID, invoice](
                                                      AbstractTransactionsCache* cache) {
                    auto parsed = ParseLnInvoice(assetID, invoice);
                    parsed->tx().set_type(lndtypes::LightingInvoiceReason::INVOICE_SWAP_PAYMENT);
                    cache->addTransactions({ parsed });
                });
            });
        });
    }
    Promise<lnrpc::AddInvoiceResponse> addInvoice(
        int64_t units, lndtypes::LightingInvoiceReason reason) override
    {
        return _proxy.addInvoice(units, reason)
            .tap([this, reason](lnrpc::AddInvoiceResponse response) {
                _proxy.lookupInvoice(response.r_hash()).then([=](lnrpc::Invoice invoice) {
                    _txCache.cacheById(_assetID).then(
                        [assetID = _assetID, invoice, reason](AbstractTransactionsCache* cache) {
                            auto parsed = ParseLnInvoice(assetID, invoice);
                            parsed->tx().set_type(reason);
                            cache->addTransactions({ parsed });
                        });
                });
            });
    }
    Promise<void> settleInvoice(
        std::string rHash, std::string rPreimage, QVariantMap payload) override
    {
        return _proxy.settleInvoice(rHash, rPreimage, payload);
    }
    Promise<void> removeInvoice(std::string rHash) override { return _proxy.removeInvoice(rHash); }
    Promise<Routes> getRoutes(swaps::u256 units, std::string destination, std::string currency,
        uint32_t finalCltvDelta) override
    {
        return _proxy.getRoutes(units, destination, currency, finalCltvDelta);
    }
    Promise<std::string> destination() const override { return _proxy.destination(); }
    Promise<uint32_t> getHeight() override { return _proxy.getHeight(); }
    uint32_t finalLock() const override { return _proxy.finalLock(); }
    swaps::ClientType type() const override { return _proxy.type(); }
    double minutesPerBlock() const override { return _proxy.minutesPerBlock(); }
    uint32_t lockBuffer() const override { return _proxy.lockBuffer(); }
    bool isConnected() const override { return _proxy.isConnected(); }
    Promise<std::vector<LndChannel>> activeChannels() const override
    {
        return _proxy.activeChannels();
    }
    Promise<LightningPayRequest> decodePayRequest(std::string paymentRequest) override
    {
        return _proxy.decodePayRequest(paymentRequest);
    }

private:
    swaps::LndSwapClient _proxy;
    AssetID _assetID;
    AssetsTransactionsCache& _txCache;
};

//==============================================================================

DexSwapClientFactory::DexSwapClientFactory(AssetsTransactionsCache& txCache,
    PaymentNodesManager& paymentNodesManager, const WalletAssetsModel& assetsModel, QObject* parent)
    : QObject(parent)
    , _txCache(txCache)
    , _paymentNodesManager(paymentNodesManager)
    , _assetsModel(assetsModel)
{
}

//==============================================================================

std::unique_ptr<swaps::AbstractSwapLndClient> DexSwapClientFactory::createLndSwapClient(
    std::string currency)
{
    auto assetID = _assetsModel.assetByName(QString::fromStdString(currency)).coinID();
    if (auto lnd = qobject_cast<LnDaemonInterface*>(_paymentNodesManager.interfaceById(assetID))) {
        return std::make_unique<LndSwapClientProxy>(lnd->grpcClient(), currency, assetID, _txCache);
    } else {
        return {};
    }
}

//==============================================================================

std::unique_ptr<swaps::AbstractSwapConnextClient> DexSwapClientFactory::createConnextSwapClient(
    std::string currency)
{
    if (currency == "WETH" || currency == "USDT" || currency == "ETH" || currency == "USDC") {
        auto asset = _assetsModel.assetByName(QString::fromStdString(currency));
        auto assetID = asset.coinID();
        auto tokenAddress
            = asset.token() ? asset.token()->contract() : asset.connextData().tokenAddress;
        if (auto connext
            = qobject_cast<ConnextDaemonInterface*>(_paymentNodesManager.interfaceById(assetID))) {
            return std::make_unique<swaps::ConnextSwapClient>(
                connext->httpClient(), tokenAddress.toStdString());
        } else {
            return {};
        }
    } else {
        return {};
    }
}

//==============================================================================

std::unique_ptr<swaps::AbstractConnextResolveService>
DexSwapClientFactory::createConnextResolveService()
{
    return std::make_unique<ConnextBrowserNodeResolverProxy>(
        _paymentNodesManager.connextDaemonManager()->browserNodeApi()->transport());
}

//==============================================================================
