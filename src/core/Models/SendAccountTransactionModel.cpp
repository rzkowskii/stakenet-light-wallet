#include "SendAccountTransactionModel.hpp"

#include <Chain/TransactionsCache.hpp>
#include <Data/WalletAssetsModel.hpp>
#include <EthCore/EthFixedGasProvider.hpp>
#include <Factories/ApiClientNetworkingFactory.hpp>
#include <Models/WalletDataSource.hpp>
#include <Networking/AbstractWeb3Client.hpp>
#include <Networking/EthGasStationHttpClient.hpp>
#include <Networking/NetworkingUtils.hpp>
#include <Tools/Common.hpp>
#include <Utils/Logging.hpp>

#include <QNetworkAccessManager>

static const std::string DEFAULT_GAS_LIMIT{ "186A0" };

//==============================================================================

SendAccountTransactionModel::SendAccountTransactionModel(AssetID assetID,
    WalletAssetsModel* walletAssetsModel, AccountDataSource* dataSource,
    qobject_delete_later_unique_ptr<AbstractWeb3Client> web3,
    AbstractTransactionsCache* transactionsCache, QObject* parent)
    : SendTransactionModel(assetID, walletAssetsModel, parent)
    , _dataSource(dataSource)
    , _transactionsCache(transactionsCache)
    , _web3(std::move(web3))
    , _assetModel(walletAssetsModel)
{
    auto chainId = *_walletAssetsModel->assetById(assetID).params().chainId;
    AbstractEthGasProvider::GasPrices fallbackPrices;
    fallbackPrices.slow = 15 * eth::shannon;
    fallbackPrices.standard = 20 * eth::shannon;
    fallbackPrices.fast = 25 * eth::shannon;
    _fallbackGasProvider = new EthFixedGasProvider(fallbackPrices, this);
    static QNetworkAccessManager accessManager;

    // if we are on mainnet use real gas station provider, otherwise use fixed price.
    _gasProvider
        = (chainId == 1) ? new EthGasStationHttpClient(&accessManager, this) : _fallbackGasProvider;
}

//==============================================================================

bool SendAccountTransactionModel::validateAddress(QString address)
{
    if (address.left(2) == "0x") {
        auto hexAddres = eth::FromHex(address);
        return hexAddres.size() == 20;
    }
    return false;
}

//==============================================================================

void SendAccountTransactionModel::createSendTransaction(const SendTransactionParams& params)
{
    Q_ASSERT_X(
        std::holds_alternative<eth::AccountSendTxParams>(params), __FUNCTION__, "Wrong tx params");
    auto ethParams = std::get<eth::AccountSendTxParams>(params);
    _dataSource->getAccountAddress(ethParams.assetID)
        .then([this, ethParams, params](QString address) mutable {
            return createSignedSendTransaction(params, address)
                .then([this, ethParams, address](eth::SignedTransaction tx) {
                    eth::AccountSendTxParams fullEthParam = ethParams;
                    fullEthParam.gasLimit = tx.gasLimit;
                    fullEthParam.gasPrice = tx.gasPrice;
                    fullEthParam.nonce = tx.nonce;
                    onSendTransactionCreated(fullEthParam, address);
                });
        })
        .tapFail([this](const std::exception& ex) {
            onSendTransactionFailed(QString::fromStdString(ex.what()));
        })
        .tapFail([this] { onSendTransactionFailed("unknown error, please try sending again"); });
}

//==============================================================================

void SendAccountTransactionModel::confirmSending()
{
    auto [params, fromAddress] = _createdTxParams;
    if (!params) {
        return;
    }

    // create again transaction to get latest nonce. We need to do this since nonce could get
    // updated at any time fetch it before sending, create new tx, sign it and broadcast.
    calculateNonce(fromAddress)
        .then([p = *params, this, from = fromAddress](int64_t nonce) mutable {
            p.nonce = ConvertFromDecimalWeiToHex(nonce);
            return _dataSource->createSendTransaction(p).then(
                [this, from](eth::SignedTransaction signedTx) {
                    return sendTransaction(from, signedTx).then([this](QString txHash) {
                        transactionSendingFinished(txHash);
                    });
                });
        })
        .fail([this](const NetworkUtils::ApiErrorException& ex) {
            transactionSendingFailed(
                QString("Transaction sending failed: %1").arg(ex.errorResponse));
        })
        .fail([this](const std::exception& ex) {
            transactionSendingFailed(QString("Transaction sending failed: %1").arg(ex.what()));
        });
}

//==============================================================================

void SendAccountTransactionModel::cancelSending()
{
    onSendTransactionCreated({}, QString{});
}

//==============================================================================

void SendAccountTransactionModel::resubmitTransaction(QString txId)
{
    _transactionsCache->ethTransactionById(txId)
        .then([](const EthOnChainTxRef& tx) {
            auto txMemo = tx->memo();
            auto it = txMemo.find("raw");
            if (it != std::end(txMemo)) {
                return QtPromise::resolve(QString::fromStdString(it->second));
            }

            throw std::runtime_error("can't resubmit tx, raw tx not available");
#if 0
        // might need to get back to this at some point
            eth::AccountSendTxParams params(tx->assetID(), tx->to(), tx->tokenAmount(),
                ConvertFromDecimalWeiToHex(tx->nonce()), eth::ConvertToHex(tx->gasPrice()),
                DEFAULT_GAS_LIMIT, tx->input(), Enums::GasType::Average);

            return _dataSource->createSendTransaction(params).then(
                [](eth::SignedTransaction signedTx) {
                    return QString::fromStdString(signedTx.toHex());
                });
#endif
        })
        .then([this](QString serializedTx) {
            return _web3->sendRawTransaction(serializedTx)
                .fail([](const NetworkUtils::ApiErrorException& ex) {
                    return Promise<QString>::reject(
                        std::runtime_error(ex.errorResponse.toStdString()));
                });
        })
        .tap([this, txId] { transactionResubmitted(txId); })
        .tapFail([this, txId](const std::runtime_error& ex) {
            transactionResubmitFailed(txId, QString::fromStdString(std::string{ ex.what() }));
        });
}

//==============================================================================

Promise<eth::SignedTransaction> SendAccountTransactionModel::createSignedSendTransaction(
    const SendTransactionParams& params, QString fromAddress) const
{
    Q_ASSERT_X(
        std::holds_alternative<eth::AccountSendTxParams>(params), __FUNCTION__, "Wrong tx params");
    auto ethParams = std::get<eth::AccountSendTxParams>(params);
    return selectGasPrice(ethParams.gasType)
        .then([ethParams, fromAddress, this](eth::u256 gasPrice) mutable {
            ethParams.gasPrice = eth::ConvertToHex(gasPrice);
            ethParams.gasLimit = DEFAULT_GAS_LIMIT;
            return calculateNonce(fromAddress)
                .then([ethParams, this, from = fromAddress](int64_t nonce) mutable {
                    ethParams.nonce = ConvertFromDecimalWeiToHex(nonce);
                    return _dataSource->createSendTransaction(ethParams);
                });
        });
}

//==============================================================================

Promise<QString> SendAccountTransactionModel::sendTransaction(
    QString fromAddress, eth::SignedTransaction transaction)
{
    LogCCInfo(General) << "Sending eth tx:" << fromAddress << transaction.to.data()
                       << transaction.data.data() << transaction.value.data()
                       << transaction.nonce.data() << transaction.gasPrice.data()
                       << transaction.gasLimit.data();
    auto rawTx = transaction.toHex();
    return _web3->sendRawTransaction(QString::fromStdString(rawTx))
        .then([this, transaction, from = fromAddress, rawTx](QString txHash) {
            TxMemo txMemo;
            txMemo.emplace(
                "decimals", QString::number(UNITS_PER_CURRENCY.at(assetID())).toStdString());
            txMemo.emplace("raw", rawTx);
            auto nonce = eth::u256{ eth::ConvertFromHex(transaction.nonce) }.convert_to<int64_t>();
            auto tx = std::make_shared<EthOnChainTx>(assetID(), txHash,
                EthOnChainTx::UNKNOWN_BLOCK_HASH, EthOnChainTx::UNKNOWN_BLOCK_HEIGHT,
                EthOnChainTx::UNKNOWN_GAS_USED, eth::ConvertFromHex(transaction.gasPrice),
                eth::ConvertFromHex(transaction.value), transaction.data, nonce, from,
                QString::fromStdString(transaction.to), QDateTime::currentDateTime(), txMemo);
            _transactionsCache->addTransactions(
                { TransactionUtils::UpdateEthTransactionType(from, false, tx) });
            return txHash;
        })
        .fail([](const NetworkUtils::ApiErrorException& ex) {
            return Promise<QString>::reject(std::runtime_error(ex.errorResponse.toStdString()));
        });
}

//==============================================================================

void SendAccountTransactionModel::onSendTransactionCreated(
    std::optional<eth::AccountSendTxParams> params, QString address)
{
    _createdTxParams = { params, address };
    if (params) {
        emit transactionCreated(params->amount * bitcoin::COIN,
            eth::ConvertFromWeiToSats(
                eth::ConvertFromHex(params->gasLimit) * eth::ConvertFromHex(params->gasPrice)),
            params->addressTo);
    }
}

//==============================================================================

void SendAccountTransactionModel::onSendTransactionFailed(QString error)
{
    _createdTxParams = std::make_tuple(std::nullopt, QString());
    emit transactionCreatingFailed(error);
}

//==============================================================================

Promise<int64_t> SendAccountTransactionModel::calculateNonce(QString address) const
{
    std::vector<Promise<int64_t>> promises;
    promises.emplace_back(
        _transactionsCache->onEthChainTransactionsList().then([address](EthOnChainTxList txns) {
            int64_t nonce = 0;
            for (auto&& tx : txns) {
                if (eth::CompareAddress(tx->from(), address)) {
                    nonce = std::max(nonce, tx->nonce());
                }
            }
            return nonce > 0 ? (nonce + 1) : 0;
        }));
    promises.emplace_back(_web3->getTransactionCount(address).then(
        [](eth::u64 nonce) { return nonce.convert_to<int64_t>(); }));
    return QtPromise::reduce(
        promises, [](auto accum, auto cur, ...) { return std::max(accum, cur); });
}

//==============================================================================

Promise<eth::u256> SendAccountTransactionModel::selectGasPrice(Enums::GasType type) const
{
    return _gasProvider->fetchGasPrice()
        .fail([this] {
            LogCCritical(Web3) << "Failed to query gas price, using fallback gas price";
            return _fallbackGasProvider->fetchGasPrice();
        })
        .then([type](AbstractEthGasProvider::GasPrices prices) {
            LogCCInfo(Web3) << "Selected gas prices: " << prices.toString()
                            << "type:" << static_cast<int>(type);
            switch (type) {
            case Enums::GasType::Slow:
                return prices.slow;
            case Enums::GasType::Average:
                return prices.standard;
            case Enums::GasType::Fast:
                return prices.fast;
            }
            return prices.standard;
        });
}

//==============================================================================
