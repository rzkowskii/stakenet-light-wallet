#include "SendUTXOTransactionModel.hpp"
#include <Chain/AbstractTransactionsCache.hpp>
#include <Data/TransactionEntry.hpp>
#include <Data/WalletAssetsModel.hpp>
#include <Factories/AbstractNetworkingFactory.hpp>
#include <Models/WalletDataSource.hpp>
#include <Networking/AbstractBlockExplorerHttpClient.hpp>
#include <Networking/RequestHandlerImpl.hpp>
#include <QJsonDocument>
#include <QJsonObject>
#include <Utils/Logging.hpp>

//==============================================================================

SendUTXOTransactionModel::SendUTXOTransactionModel(AssetID assetID,
    WalletAssetsModel* walletAssetsModel, WalletDataSource* dataSource,
    AbstractNetworkingFactory* apiClientFactory, AbstractTransactionsCache* transactionsCache,
    QObject* parent)
    : SendTransactionModel(assetID, walletAssetsModel, parent)
    , _walletDataSource(dataSource)
    , _transactionsCache(transactionsCache)

{
    _apiClient = apiClientFactory->createBlockExplorerClient(this->assetID());
    requestAddressDetails();
}

//==============================================================================

bool SendUTXOTransactionModel::validateAddress(QString address)
{
    auto it = std::find_if(
        _addressesEntry.begin(), _addressesEntry.end(), [address](const auto& addressEntry) {
            if (address.length() > addressEntry.second) {
                return false;
            }
            auto prefixes = addressEntry.first;
            auto it
                = std::find_if(prefixes.begin(), prefixes.end(), [address](const QString& prefix) {
                      if (address.length() < prefix.length()) {
                          return prefix.startsWith(address);
                      } else {
                          return address.startsWith(prefix);
                      }
                  });

            return it != prefixes.end();
        });

    return it != _addressesEntry.end();
}

//==============================================================================

void SendUTXOTransactionModel::createSendTransaction(const SendTransactionParams& params)
{
    Q_ASSERT_X(
        std::holds_alternative<bitcoin::UTXOSendTxParams>(params), __FUNCTION__, "Wrong tx params");
    _walletDataSource->createSendTransaction(std::get<bitcoin::UTXOSendTxParams>(params))
        .then([this](MutableTransactionRef transaction) { onSendTransactionCreated(transaction); })
        .fail([this](const std::string& error) {
            transactionCreatingFailed(QString::fromStdString(error));
        });
}

//==============================================================================

void SendUTXOTransactionModel::confirmSending()
{
    if (!_transactionsCache) {
        LogCCritical(General) << "Transaction cache is null, unexpected";
        return;
    }

    LogCDebug(General) << "Sending tx:" << _createdTx->txId().c_str();
    auto serializedTx = QString::fromStdString(_createdTx->serialized());

    _apiClient->sendTransaction(serializedTx)
        .then([this](QString transactionID) {
            auto ref
                = TransactionUtils::MutableTxToTransactionRef(*_createdTx, *_walletAssetsModel);
            _walletDataSource->applyTransactionAsync(ref).then(
                [this, transactionID](OnChainTxRef appliedTx) {
                    _transactionsCache->addTransactions({ appliedTx });
                    transactionSendingFinished(transactionID);
                });
        })
        .fail([this] { transactionSendingFailed("Transaction sending failed"); });
}

//==============================================================================

void SendUTXOTransactionModel::cancelSending()
{
    onSendTransactionCreated(MutableTransactionRef());
}

//==============================================================================

void SendUTXOTransactionModel::resubmitTransaction(QString txId)
{
    Q_UNUSED(txId);
    // TODO(yuraolex): not yet supported for UTXO transactions
}

//==============================================================================

void SendUTXOTransactionModel::onSendTransactionCreated(MutableTransactionRef transaction)
{
    _createdTx = transaction;
    if (_createdTx != nullptr) {
        auto outputs = _createdTx->outputs();
        auto changePos = _createdTx->changePos();

        qint64 amount = std::accumulate(outputs.begin(), outputs.end(), 0ll,
            [](qint64 value, const auto& out) { return value + std::get<1>(out); });

        if (changePos >= 0) {
            amount -= std::get<1>(_createdTx->outputs().at(changePos));
        }

        transactionCreated(amount, _createdTx->fee(), _createdTx->recipientAddress());
    }
}

//==============================================================================

void SendUTXOTransactionModel::requestAddressDetails()
{
    auto chainParams = _walletAssetsModel->assetById(assetID()).params().params;
    _addressesEntry.clear();
    auto addressesDetails = GetAddressDetails();

    for (auto& type : chainParams.base58Types()) {
        auto& decimalPrefixes = chainParams.base58Prefix(type);
        std::transform(decimalPrefixes.begin(), decimalPrefixes.end(),
            std::back_inserter(_addressesEntry), [addressesDetails](const auto& prefix) {
                if (prefix >= MAX_PREFIX) {
                    return addressesDetails.at(MAX_PREFIX - 1);
                }
                return addressesDetails.at(prefix);
            });
    }

    auto bech32Prefix = chainParams.bech32HRP();
    if (!bech32Prefix.empty()) {
        _addressesEntry.push_back(AddressEntry({ QString::fromStdString(bech32Prefix) }, 90));
    }
}

//==============================================================================
