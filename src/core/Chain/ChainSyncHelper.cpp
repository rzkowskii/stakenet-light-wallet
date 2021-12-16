#include "ChainSyncHelper.hpp"
#include <Chain/Chain.hpp>
#include <Data/TransactionEntry.hpp>
#include <Utils/Logging.hpp>
#include <utilstrencodings.h>

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>

static const QString ORDER = "asc";
static constexpr const size_t CHAIN_HEADERS_SYNC_LIMIT = 1000;
static constexpr const size_t BLOCK_TRANSACTIONS_SYNC_LIMIT = 100;

//==============================================================================

static void FromJson(chain::TxOutput& output, const QJsonObject& obj)
{
    auto address = obj.value("address").toString();

    if (address.isEmpty()) {
        address = obj.value("addresses").toArray().first().toString();
    }

    output.set_index(obj.value("index").toInt());
    output.set_value(ParseAmount(obj.value("value")));
    output.set_address(address.toStdString());
}

//==============================================================================

static void FromJson(chain::TxOutpoint& input, const QJsonObject& obj)
{
    input.set_hash(obj.value("txid").toString().toStdString());
    input.set_index(obj.value("index").toInt());
}

//==============================================================================

static void OnGetBlockTxResponseFinished(
    const QtPromise::QPromiseResolve<Wire::StrippedBlock>& resolve,
    const QtPromise::QPromiseReject<Wire::StrippedBlock>& reject, AssetID assetID,
    Wire::StrippedBlock block, int64_t blockHeight, QByteArray response,
    AbstractBlockExplorerHttpClient* apiClient)
{
    std::vector<OnChainTxRef> parsed;
    QJsonDocument doc = QJsonDocument::fromJson(response);
    QJsonValue data = doc.object().value("data");
    QJsonArray array = data.toArray();
    auto blockHash = block.hash;

    if (!array.isEmpty()) {
        for (int i = 0; i < array.size(); ++i) {
            auto obj = array.at(i).toObject();

            auto inputsJson = obj.value("inputs").toArray();
            OnChainTx::Inputs inputs;
            inputs.reserve(inputsJson.size());
            for (auto input : inputsJson) {
                inputs.emplace_back(chain::TxOutpoint{});
                FromJson(inputs.back(), input.toObject());
            }

            auto outputsJson = obj.value("outputs").toArray();
            OnChainTx::Outputs outputs;
            outputs.reserve(outputsJson.size());
            for (auto output : outputsJson) {
                outputs.emplace_back(chain::TxOutput{});
                FromJson(outputs.back(), output.toObject());
            }

            parsed.emplace_back(std::make_shared<OnChainTx>(assetID, obj.value("id").toString(),
                blockHash, blockHeight, static_cast<uint32_t>(i),
                QDateTime::fromSecsSinceEpoch(static_cast<int64_t>(obj.value("time").toDouble())),
                inputs, outputs,
                chain::OnChainTransaction_TxType::OnChainTransaction_TxType_PAYMENT, TxMemo{}));
        }

        block.addTransactions(parsed);
        QString lastTxId = array.last().toObject().value("id").toString();
        apiClient->getBlockTxByHash(blockHash, lastTxId, BLOCK_TRANSACTIONS_SYNC_LIMIT)
            .then([=](QByteArray response) {
                OnGetBlockTxResponseFinished(
                    resolve, reject, assetID, block, blockHeight, response, apiClient);
            })
            .fail(reject);
    } else {
        resolve(block);
    }
}

//==============================================================================

ChainSyncHelper::ChainSyncHelper(QPointer<AbstractBlockExplorerHttpClient> blockExplorerHttpClient,
    CoinAsset asset, QObject* parent)
    : QObject(parent)
    , _blockExplorerHttpClient(blockExplorerHttpClient)
    , _asset(asset)
{
    Q_ASSERT(_blockExplorerHttpClient);
}

//==============================================================================

ChainSyncHelper::~ChainSyncHelper() {}

//==============================================================================

void ChainSyncHelper::getBestBlockHash()
{
    _blockExplorerHttpClient->getBestBlockHash()
        .then([=](QByteArray response) { onGetBestBlockHashResponseFinished(response); })
        .fail([this](const NetworkUtils::ApiErrorException& error) {
            onGetBestBlockHashFailed(error);
        });
}

//==============================================================================

Promise<Wire::StrippedBlock> ChainSyncHelper::getLightWalletBlock(QString hash, int64_t blockHeight)
{
    return Promise<Wire::StrippedBlock>([=](const auto& resolve, const auto& reject) {
        _blockExplorerHttpClient->getBlockTxByHash(hash, QString(), BLOCK_TRANSACTIONS_SYNC_LIMIT)
            .then([=](QByteArray response) {
                OnGetBlockTxResponseFinished(resolve, reject, _asset.coinID(),
                    Wire::StrippedBlock(hash), blockHeight, response, _blockExplorerHttpClient);
            })
            .fail([reject](const NetworkUtils::ApiErrorException& err) {
                reject(err);
            });
    });
}

//==============================================================================

AssetID ChainSyncHelper::assetId() const
{
    return _asset.coinID();
}

//==============================================================================

Promise<std::vector<Wire::VerboseBlockHeader>> ChainSyncHelper::getLastHeaders(QString lastHeaderId)
{
    return Promise<std::vector<Wire::VerboseBlockHeader>>(
        [=](const auto& resolve, const auto& reject) {
            _blockExplorerHttpClient->getBlockHeaders(lastHeaderId, CHAIN_HEADERS_SYNC_LIMIT)
                .then([=](QByteArray response) {
                    QJsonDocument doc = QJsonDocument::fromJson(response);
                    QJsonArray data = doc.object().value("data").toArray();
                    std::vector<Wire::VerboseBlockHeader> result;

                    std::transform(std::begin(data), std::end(data), std::back_inserter(result),
                        [](const QJsonValue& value) {
                            return Wire::VerboseBlockHeader::FromJson(value.toObject());
                        });

                    resolve(result);
                })
                .fail([reject, lastHeaderId](
                          const NetworkUtils::ApiErrorException& error) {
                    LogCDebug(Api) << "Failed to get headers" << lastHeaderId;

                    reject(error);
                });
        });
}

//==============================================================================

Promise<BlockHash> ChainSyncHelper::getBlockHash(unsigned int blockIndex) const
{
    return _blockExplorerHttpClient->getBlockHashByHeight(blockIndex).then([](QByteArray response) {
        //        QJsonObject jsonObject =
        //        QJsonDocument::fromJson(response).object()["block"].toObject(); return
        //        jsonObject.value("hash").toString();
        QJsonObject jsonObject = QJsonDocument::fromJson(response).object();
        return jsonObject.value("hash").toString();
    });
}

//==============================================================================

void ChainSyncHelper::onGetBestBlockHashResponseFinished(QByteArray response)
{
    QJsonDocument doc = QJsonDocument::fromJson(response);
    QJsonArray data = doc.array();
    auto obj = data.first().toObject();
    bestBlockSynced(obj.value("hash").toString(), static_cast<size_t>(obj.value("height").toInt()));
}

//==============================================================================

void ChainSyncHelper::onGetBestBlockHashFailed(
    const NetworkUtils::ApiErrorException& error)
{
    LogCDebug(General) << "Failed to get best block hash";
    bestBlockHashFailed(error.errorResponse);
}

//==============================================================================
