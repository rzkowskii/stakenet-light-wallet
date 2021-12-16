#include "AccountSyncHelper.hpp"
#include <Tools/Common.hpp>
#include <Utils/Logging.hpp>

#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

using namespace boost::adaptors;

static constexpr const size_t ACCOUNT_TX_SYNC_LIMIT = 1000;

//==============================================================================

static void onGetTransactionsResponseFinished(
    const QtPromise::QPromiseResolve<EthOnChainTxList>& resolve,
    const QtPromise::QPromiseReject<EthOnChainTxList>& reject, EthOnChainTxList transactions,
    QByteArray response, AbstractAccountExplorerHttpClient* apiClient, AssetID assetID,
    std::vector<CoinAsset> activeTokens, QString address, QString lastSyncedTxId)
{
    QJsonDocument doc = QJsonDocument::fromJson(response);
    QJsonValue data = doc.object().value("data");

    auto scrollId = doc.object().value("scrollId").toString();
    QJsonArray array = data.toArray();
    if (!array.isEmpty()) {
        EthOnChainTxList temp;
        bool synced = false;

        auto transform = [assetID, activeTokens](const auto& value) {
            auto obj = value.toObject().toVariantMap();
            auto ethDetails
                = AccountSyncHelper::ethDetails(assetID, obj.value("to").toString(), activeTokens);
            return AccountSyncHelper::ParseTx(
                std::get<0>(ethDetails), std::get<1>(ethDetails), obj);
        };

        boost::copy(array | transformed(transform), std::back_inserter(temp));

        for (const auto &transaction : temp) {
            transactions.insert(transactions.begin(), transaction); // for right order

            if ((!lastSyncedTxId.isEmpty() && transaction->txId() == lastSyncedTxId)
                || (lastSyncedTxId.isEmpty() && transaction->txId() == scrollId)) {
                synced = true;
                break;
            }
        }

        if (!synced) {
            apiClient->getAccountTransactionsForAddress(address, ACCOUNT_TX_SYNC_LIMIT, scrollId)
                .then([=](QByteArray response) {
                    onGetTransactionsResponseFinished(resolve, reject, transactions, response,
                        apiClient, assetID, activeTokens, address, lastSyncedTxId);
                })
                .fail(reject);
        } else {
            resolve(transactions);
        }
    } else {
        resolve(transactions);
    }
}
//==============================================================================

AccountSyncHelper::AccountSyncHelper(
    QPointer<AbstractAccountExplorerHttpClient> accountExplorerHttpClient, CoinAsset asset,
    std::vector<CoinAsset> activeTokens, QObject* parent)
    : QObject(parent)
    , _accountExplorerHttpClient(accountExplorerHttpClient)
    , _asset(asset)
    , _activeTokens(activeTokens)
{
    Q_ASSERT(accountExplorerHttpClient);
}

//==============================================================================

AccountSyncHelper::~AccountSyncHelper() {}

//==============================================================================

Promise<EthOnChainTxList> AccountSyncHelper::getTransactions(
    QString address, QString lastSyncedTxId)
{
    return Promise<EthOnChainTxList>([=](const auto& resolve, const auto& reject) {
        _accountExplorerHttpClient->getAccountTransactionsForAddress(address, ACCOUNT_TX_SYNC_LIMIT)
            .then([=](QByteArray response) {
                onGetTransactionsResponseFinished(resolve, reject, EthOnChainTxList(), response,
                    _accountExplorerHttpClient, _asset.coinID(), _activeTokens, address,
                    lastSyncedTxId);
            })
            .fail([reject](const NetworkUtils::ApiErrorException& error) {
                LogCDebug(Api) << "Failed to get account transactions";

                reject(error);
            });
    });
}

//==============================================================================

bool AccountSyncHelper::IsRemoteTxConfirmed(const QVariantMap& remoteTx)
{
    auto blockHash = remoteTx.value("blockHash", EthOnChainTx::UNKNOWN_BLOCK_HASH).toString();
    auto blockNumber = static_cast<int64_t>(
        remoteTx.value("blockNumber", static_cast<double>(EthOnChainTx::UNKNOWN_BLOCK_HEIGHT))
            .toDouble());

    return (blockHash != EthOnChainTx::UNKNOWN_BLOCK_HASH
        && blockNumber != EthOnChainTx::UNKNOWN_BLOCK_HEIGHT);
}

//==============================================================================

EthOnChainTxRef AccountSyncHelper::MergeTransactions(
    const EthOnChainTxRef& localTx, const QVariantMap& remoteTx)
{
    auto txHash = remoteTx.value("hash").toString();
    Q_ASSERT_X(txHash == localTx->txId(), Q_FUNC_INFO,
        "Merging transactions are supported only between same transactions");

    auto blockHash = remoteTx.value("blockHash", EthOnChainTx::UNKNOWN_BLOCK_HASH).toString();
    auto blockNumberVar = remoteTx.value("blockNumber");

    if (blockHash == EthOnChainTx::UNKNOWN_BLOCK_HASH || !blockNumberVar.isValid()) {
        return localTx;
    }

    auto gasUsed = remoteTx
                       .value("gas",
                           QString::fromStdString(
                               ConvertFromDecimalWeiToHex(EthOnChainTx::UNKNOWN_GAS_USED)))
                       .toString()
                       .toStdString();

    auto timestampVar = remoteTx.value("timestamp");
    auto timestamp = localTx->transactionDate();
    if (timestampVar.isValid()) {
        timestamp = QDateTime::fromSecsSinceEpoch(static_cast<int64_t>(timestampVar.toDouble()));
    }

    auto txMemo = localTx->memo();
    auto transactionIndexVar = remoteTx.value("transactionIndex");
    if (transactionIndexVar.isValid()) {
        txMemo.emplace("transactionIndex",
            QString::number(static_cast<int32_t>(transactionIndexVar.toInt())).toStdString());
    }

    auto blockNumber = eth::u256{ blockNumberVar.toString().toStdString() }.convert_to<int64_t>();

    return std::make_shared<EthOnChainTx>(localTx->assetID(), localTx->txId(), blockHash,
        blockNumber, eth::u256{ gasUsed }, localTx->gasPrice(), localTx->value(), localTx->input(),
        localTx->nonce(), localTx->from(), localTx->to(), timestamp, txMemo);
}

//==============================================================================

std::tuple<AssetID, uint32_t> AccountSyncHelper::ethDetails(
    AssetID assetID, QString addressTo, std::vector<CoinAsset> activeTokens)
{
    auto it = std::find_if(
        activeTokens.begin(), activeTokens.end(), [addressTo](const CoinAsset& coinAsset) {
            return coinAsset.token()->contract().toLower() == addressTo;
        });

    return it != activeTokens.end()
        ? std::make_tuple(std::move(it->coinID()), it->token()->decimals())
        : std::make_tuple(std::move(assetID), uint32_t(18));
};

//==============================================================================

EthOnChainTxRef AccountSyncHelper::ParseTx(AssetID assetID, uint32_t decimals, QVariantMap obj)
{
    TxMemo txMemo;
    auto transactionIndexVar = obj.value("transactionIndex");
    if (transactionIndexVar.isValid()) {
        txMemo.emplace("transactionIndex",
            QString::number(static_cast<int32_t>(transactionIndexVar.toInt())).toStdString());
    }

    auto blockNumber = static_cast<int64_t>(
        obj.value("blockNumber", static_cast<double>(EthOnChainTx::UNKNOWN_BLOCK_HEIGHT))
            .toDouble());

    txMemo.emplace("decimals", QString::number(decimals).toStdString());
    auto gasUsed = ConvertFromDecimalWeiToHex(obj.value("gas").toDouble());
    auto gasPrice = ConvertFromDecimalWeiToHex(obj.value("gasPrice").toDouble());
    auto value = ConvertFromDecimalWeiToHex(obj.value("value").toDouble());
    auto nonce = static_cast<int64_t>(obj.value("nonce").toDouble());
    auto timestamp
        = QDateTime::fromSecsSinceEpoch(static_cast<int64_t>(obj.value("timestamp").toDouble()));

    return std::make_shared<EthOnChainTx>(assetID, obj.value("hash").toString(),
        obj.value("blockHash", EthOnChainTx::UNKNOWN_BLOCK_HASH).toString(), blockNumber,
        eth::u256{ gasUsed }, eth::u256{ gasPrice }, eth::u256{ value },
        obj.value("input").toString().toStdString(), nonce, obj.value("from").toString(),
        obj.value("to").toString(), timestamp, txMemo);
};

//==============================================================================
