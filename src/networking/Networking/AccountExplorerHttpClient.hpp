#ifndef ACCOUNTEXPLORERHTTPCLIENT_HPP
#define ACCOUNTEXPLORERHTTPCLIENT_HPP

#include "AbstractAccountExplorerHttpClient.hpp"
#include "RequestHandlerImpl.hpp"
#include <QObject>
#include <QPointer>
#include <memory>

class AccountExplorerHttpClient : public AbstractAccountExplorerHttpClient {
    Q_OBJECT
public:
    AccountExplorerHttpClient(std::unique_ptr<RequestHandlerImpl> &&requestHandler, QObject* parent = nullptr);

public slots:
    Promise<QByteArray> getAccountTransactionsForAddress(QString address, size_t limit, QString lastSeenTxHash = QString()) override;

private:
    RequestHandlerImpl* _requestHandler{ nullptr };
};

#endif // ACCOUNTEXPLORERHTTPCLIENT_HPP
