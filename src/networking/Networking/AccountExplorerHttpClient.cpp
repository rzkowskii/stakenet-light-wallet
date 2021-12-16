#include "AccountExplorerHttpClient.hpp"
#include <Networking/NetworkingUtils.hpp>
#include <Utils/Logging.hpp>

//==============================================================================

AccountExplorerHttpClient::AccountExplorerHttpClient(
    std::unique_ptr<RequestHandlerImpl>&& requestHandler, QObject* parent)
    : AbstractAccountExplorerHttpClient(parent)
{
    requestHandler->setParent(this);
    _requestHandler = requestHandler.release();
}

//==============================================================================

Promise<QByteArray> AccountExplorerHttpClient::getAccountTransactionsForAddress(QString address, size_t limit, QString lastSeenTxHash)
{
    Q_ASSERT(!address.isEmpty());

    return Promise<QByteArray>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(this, [=] {
            auto errorHandler = [=](int errorCode, const QString& errorResponse) {
                reject(NetworkUtils::ApiErrorException{ errorCode, errorResponse });
            };

            auto responseHandler = [=](const QByteArray& response) { resolve(response); };

            QVariantMap params;

            if (!lastSeenTxHash.isEmpty()) {
                params.insert("scrollId", lastSeenTxHash);
                params.insert("limit", QString::number(limit));
            }

            const QString url = QString("/addresses/%1/transactions")
                                    .arg(QString(QUrl::toPercentEncoding(address.toLower())));

            _requestHandler->makeGetRequest(url, params, errorHandler, responseHandler);
        });
    });
}

//==============================================================================
