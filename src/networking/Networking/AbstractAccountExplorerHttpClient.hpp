#ifndef ABSTRACTACCOUNTEXPLORERHTTPCLIENT_HPP
#define ABSTRACTACCOUNTEXPLORERHTTPCLIENT_HPP

#include <QObject>
#include <QtPromise>
#include <Utils/Utils.hpp>

class AbstractAccountExplorerHttpClient : public QObject {
    Q_OBJECT
public:
    explicit AbstractAccountExplorerHttpClient(QObject* parent = nullptr);
    virtual ~AbstractAccountExplorerHttpClient();

    template <class T> using Promise = QtPromise::QPromise<T>;

    virtual Promise<QByteArray> getAccountTransactionsForAddress(QString address, size_t limit, QString lastSeenTxHash = QString()) = 0;
};

#endif // ABSTRACTACCOUNTEXPLORERHTTPCLIENT_HPP
