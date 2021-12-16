#ifndef ABSTRACTBLOCKEXPLORERHTTPCLIENT_HPP
#define ABSTRACTBLOCKEXPLORERHTTPCLIENT_HPP

#include <QObject>
#include <QtPromise>
#include <Utils/Utils.hpp>

class AbstractBlockExplorerHttpClient : public QObject {
    Q_OBJECT
public:
    explicit AbstractBlockExplorerHttpClient(QObject* parent = nullptr);
    virtual ~AbstractBlockExplorerHttpClient();

    template <class T> using Promise = QtPromise::QPromise<T>;

    virtual Promise<QByteArray> getTransactionsForAddress(
        QString address, size_t limit, QString order, QString lastSeenTxid)
        = 0;
    virtual Promise<QByteArray> getBlockHeaders(QString lastSeenId, size_t limit) = 0;
    virtual Promise<QByteArray> getBlockHeader(QString hash) = 0;
    virtual Promise<QByteArray> getBlock(QString hash) = 0;
    virtual Promise<QByteArray> getBestBlockHash() = 0;
    virtual Promise<QByteArray> getBlockTxByHash(QString hash, QString lastSeenTxId, size_t limit)
        = 0;
    virtual Promise<QByteArray> getTransaction(QString transactionHash) = 0;
    virtual Promise<QByteArray> getRawTransaction(QString transactionHash) = 0;
    virtual Promise<QByteArray> getRawTxByIndex(int64_t blockNum, uint32_t txIndex) = 0;
    virtual Promise<QString> sendTransaction(QString hexEncodedTx) = 0;
    virtual Promise<double> estimateSmartFee(unsigned blocksTarget) = 0;
    virtual Promise<QByteArray> getBlockHashByHeight(unsigned blockHeight) = 0;
    virtual Promise<QByteArray> getBlockFilter(QString blockHash) = 0;
    virtual Promise<QByteArray> getTxOut(QString txid, unsigned int outputIndex) = 0;
    virtual Promise<QByteArray> getSpendingTx(QString txHash, unsigned int outputIndex) = 0;
};

#endif // ABSTRACTBLOCKEXPLORERHTTPCLIENT_HPP
