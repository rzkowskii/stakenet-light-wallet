#ifndef ACCOUNTSYNCHELPER_HPP
#define ACCOUNTSYNCHELPER_HPP

#include <Data/CoinAsset.hpp>
#include <Data/TransactionEntry.hpp>
#include <Networking/AbstractAccountExplorerHttpClient.hpp>
#include <Networking/NetworkingUtils.hpp>
#include <QObject>
#include <QPointer>
#include <Utils/Utils.hpp>

class AbstractAccountExplorerHttpClient;

class AccountSyncHelper : public QObject {
    Q_OBJECT
public:
    explicit AccountSyncHelper(
        QPointer<AbstractAccountExplorerHttpClient> accountExplorerHttpClient, CoinAsset asset,
        std::vector<CoinAsset> activeTokens, QObject* parent = nullptr);
    ~AccountSyncHelper();

    Promise<EthOnChainTxList> getTransactions(QString address, QString lastSyncedTxId);

    // IsRemoteTxConfirmed checks if remote transaction is confirmed
    static bool IsRemoteTxConfirmed(const QVariantMap& remoteTx);
    // MergeTransactions is used to merge our local tx with one that we have received from remote.
    static EthOnChainTxRef MergeTransactions(
        const EthOnChainTxRef& localTx, const QVariantMap& remoteTx);
    // ParseTx parses transaction that we have received from remote source such as web3 API or block
    // explorer API.
    static EthOnChainTxRef ParseTx(AssetID assetID, uint32_t decimals, QVariantMap obj);
    // 0 - assetID, 1 - decimals
    static std::tuple<AssetID, uint32_t> ethDetails(
        AssetID assetID, QString addressTo, std::vector<CoinAsset> activeTokens);

signals:
    void transactionSynced(int64_t countTx, int64_t currentTxIndex);

private:
    QPointer<AbstractAccountExplorerHttpClient> _accountExplorerHttpClient;
    CoinAsset _asset;
    std::vector<CoinAsset> _activeTokens;
};

#endif // ACCOUNTSYNCHELPER_HPP
