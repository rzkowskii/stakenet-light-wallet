#ifndef ABSTRACTWEB3CLIENT_HPP
#define ABSTRACTWEB3CLIENT_HPP

#include <QObject>
#include <QtPromise>

#include <EthCore/Types.hpp>

class SubcribeEventEmitter;

class AbstractWeb3Client : public QObject {
    Q_OBJECT
public:
    explicit AbstractWeb3Client(QObject* parent = nullptr);

    template <class T> using Promise = QtPromise::QPromise<T>;

    virtual void open() = 0;

    virtual Promise<eth::u256> getBalance(QString address) = 0;
    virtual Promise<eth::u64> getBlockNumber() = 0;
    virtual Promise<QVariantMap>getBlockByNumber(QString blockNumber) = 0;
    virtual Promise<QString> call(QVariantMap params) = 0;
    virtual Promise<eth::u256> getChainId() = 0;
    virtual Promise<eth::u64> estimateGas(QVariantMap params) = 0;
    virtual Promise<eth::u64> getTransactionCount(QString addressTo) = 0;
    virtual Promise<QVariantMap> getTransactionByHash(QString txHash) = 0;
    virtual Promise<QVariantMap> getTransactionReceipt(QString txHash) = 0;

    virtual Promise<QString> sendRawTransaction(QString eventType) = 0;
    virtual Promise<void> subscribe(QString serializedTxHex) = 0;

signals:
    void connected();
};

#endif // ABSTRACTWEB3CLIENT_HPP
