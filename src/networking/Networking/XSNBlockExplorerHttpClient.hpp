#ifndef XSNBlockExplorerHttpClient_HPP
#define XSNBlockExplorerHttpClient_HPP

#include "AbstractBlockExplorerHttpClient.hpp"
#include "RequestHandlerImpl.hpp"
#include <QObject>
#include <QPointer>
#include <memory>

class XSNBlockExplorerHttpClient : public AbstractBlockExplorerHttpClient {
    Q_OBJECT
public:
    XSNBlockExplorerHttpClient(
        std::unique_ptr<RequestHandlerImpl>&& requestHandler, QObject* parent = nullptr);

public slots:
    Promise<QByteArray> getTransactionsForAddress(
        QString address, size_t limit, QString order, QString lastSeenTxid) override;
    Promise<QByteArray> getBlockHeaders(QString lastSeenId, size_t limit) override;
    Promise<QByteArray> getBlockHeader(QString hash) override;
    Promise<QByteArray> getBlock(QString hash) override;
    Promise<QByteArray> getBestBlockHash() override;
    Promise<QByteArray> getBlockTxByHash(QString hash, QString lastSeenTxId, size_t limit) override;
    Promise<QByteArray> getTransaction(QString transactionHash) override;
    Promise<QString> sendTransaction(QString hexEncodedTx) override;
    Promise<double> estimateSmartFee(unsigned blocksTarget) override;
    Promise<QByteArray> getBlockHashByHeight(unsigned blockHeight) override;
    Promise<QByteArray> getRawTransaction(QString transactionHash) override;
    Promise<QByteArray> getRawTxByIndex(int64_t blockNum, uint32_t txIndex) override;
    Promise<QByteArray> getBlockFilter(QString blockHash) override;
    Promise<QByteArray> getTxOut(QString txid, unsigned int outputIndex) override;
    Promise<QByteArray> getSpendingTx(QString txHash, unsigned int outputIndex) override;

private:
    RequestHandlerImpl* _requestHandler{ nullptr };
};

#endif // XSNBlockExplorerHttpClient_HPP
