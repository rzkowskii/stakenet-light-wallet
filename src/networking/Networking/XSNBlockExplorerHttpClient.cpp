#include "XSNBlockExplorerHttpClient.hpp"
#include <Networking/NetworkingUtils.hpp>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <Utils/Logging.hpp>

//==============================================================================

template <class F>
void MakeRequest(
    QObject* self, QNetworkAccessManager* accessManager, QNetworkRequest request, F resolve)
{
    auto reply = accessManager->get(request);
    QObject::connect(reply, &QNetworkReply::finished, self, [=] {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            LogCDebug(Api) << "Error:" << reply->errorString();
            MakeRequest(self, accessManager, reply->request(), resolve);
        } else {
            resolve(reply->readAll());
        }
    });
}

//==============================================================================

XSNBlockExplorerHttpClient::XSNBlockExplorerHttpClient(
    std::unique_ptr<RequestHandlerImpl>&& requestHandler, QObject* parent)
    : AbstractBlockExplorerHttpClient(parent)
{
    requestHandler->setParent(this);
    _requestHandler = requestHandler.release();
}

//==============================================================================

auto XSNBlockExplorerHttpClient::getTransactionsForAddress(
    QString address, size_t limit, QString order, QString lastSeenTxid) -> Promise<QByteArray>
{
    Q_ASSERT(!address.isEmpty());
    return Promise<QByteArray>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(this, [=] {
            auto errorHandler = [=](int errorCode, const QString& errorResponse) {
                reject(NetworkUtils::ApiErrorException{ errorCode, errorResponse });
            };

            auto responseHandler = [=](const QByteArray& response) { resolve(response); };

            QVariantMap params = { { "limit", QString::number(limit) }, { "order", order } };

            if (!lastSeenTxid.isEmpty()) {
                params.insert("lastSeenTxid", lastSeenTxid);
            }

            const QString url = QString("/v2/addresses/%1/transactions")
                                    .arg(QString(QUrl::toPercentEncoding(address)));

            // LogCDebug(Api) << "Making get request:" << url;

            _requestHandler->makeGetRequest(url, params, errorHandler, responseHandler);
        });
    });
}

//==============================================================================

auto XSNBlockExplorerHttpClient::getBlockHeaders(QString lastSeenId, size_t limit)
    -> Promise<QByteArray>
{
    return Promise<QByteArray>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(this, [=] {
            auto errorHandler = [=](int errorCode, const QString& errorResponse) {
                reject(NetworkUtils::ApiErrorException{ errorCode, errorResponse });
            };

            auto responseHandler = [=](const auto& response) { resolve(response); };

            QVariantMap params;
            if (!lastSeenId.isEmpty()) {
                params.insert("lastSeenHash", lastSeenId);
            }
            params.insert("limit", QString::number(limit));
            params.insert("includeFilter", true);

            const QString url = QString("/block-headers");
            _requestHandler->makeGetRequest(url, params, errorHandler, responseHandler);
        });
    });
}

//==============================================================================

auto XSNBlockExplorerHttpClient::getBlockHeader(QString hash) -> Promise<QByteArray>
{
    return Promise<QByteArray>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(this, [=] {
            auto errorHandler = [=](int errorCode, const QString& errorResponse) {
                reject(NetworkUtils::ApiErrorException{ errorCode, errorResponse });
            };

            auto responseHandler = [=](const QByteArray& response) { resolve(response); };

            QVariantMap params;
            params["includeFilter"] = true;

            const QString url = QString("/block-headers/%1").arg(hash);
            _requestHandler->makeGetRequest(url, params, errorHandler, responseHandler);
        });
    });
}

//==============================================================================

auto XSNBlockExplorerHttpClient::getBlock(QString hash) -> Promise<QByteArray>
{
    return Promise<QByteArray>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(this, [=] {
            auto errorHandler = [=](int errorCode, const QString& errorResponse) {
                reject(NetworkUtils::ApiErrorException{ errorCode, errorResponse });
            };

            auto responseHandler = [=](const QByteArray& response) { resolve(response); };

            QString url = QString("/v2/blocks/%1/hex").arg(hash);

            _requestHandler->makeGetRequest(url, QVariantMap(), errorHandler, responseHandler);
        });
    });
}

//==============================================================================

auto XSNBlockExplorerHttpClient::getBestBlockHash() -> Promise<QByteArray>
{
    return Promise<QByteArray>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(this, [=] {
            auto errorHandler = [=](int errorCode, const QString& errorResponse) {
                reject(NetworkUtils::ApiErrorException{ errorCode, errorResponse });
            };

            auto responseHandler = [=](const QByteArray& response) { resolve(response); };

            _requestHandler->makeGetRequest(
                "/blocks", QVariantMap(), errorHandler, responseHandler);
        });
    });
}

//==============================================================================

auto XSNBlockExplorerHttpClient::getBlockTxByHash(QString hash, QString lastSeenTxId, size_t limit)
    -> Promise<QByteArray>
{
    return Promise<QByteArray>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(this, [=] {
            auto errorHandler = [=](int errorCode, const QString& errorResponse) {
                reject(NetworkUtils::ApiErrorException{ errorCode, errorResponse });
            };

            auto responseHandler = [=](const QByteArray& response) { resolve(response); };

            QVariantMap params;
            if (!lastSeenTxId.isEmpty()) {
                params.insert("lastSeenTxid", lastSeenTxId);
            }
            params.insert("limit", QString::number(limit));

            QString url = QString("/v2/blocks/%1/light-wallet-transactions").arg(hash);
            _requestHandler->makeGetRequest(url, params, errorHandler, responseHandler);
        });
    });
}

//==============================================================================

auto XSNBlockExplorerHttpClient::getTransaction(QString transactionHash) -> Promise<QByteArray>
{
    Q_ASSERT(!transactionHash.isEmpty());

    return Promise<QByteArray>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(this, [=] {
            auto errorHandler = [=](int errorCode, const QString& errorResponse) {
                reject(NetworkUtils::ApiErrorException{ errorCode, errorResponse });
            };

            auto responseHandler = [=](const QByteArray& response) { resolve(response); };

            const QString url
                = QString("transactions").arg(QString(QUrl::toPercentEncoding(transactionHash)));
            _requestHandler->makeGetRequest(url, {}, errorHandler, responseHandler);
        });
    });
}

//==============================================================================

auto XSNBlockExplorerHttpClient::sendTransaction(QString hexEncodedTx) -> Promise<QString>
{
    Q_ASSERT(!hexEncodedTx.isEmpty());
    return Promise<QString>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(this, [=] {
            auto errorHandler = [=](int errorCode, const QString& errorResponse) {
                reject(NetworkUtils::ApiErrorException{ errorCode, errorResponse });
            };

            auto responseHandler = [=](const QByteArray& response) {
                LogCDebug(WalletBackend) << "Sucessfully sent, response:" << response;
                QJsonDocument doc = QJsonDocument::fromJson(response);
                resolve(doc.object().value("txid").toString());
            };

            QJsonDocument requestBody;
            const QString url = QString("/transactions");
            QJsonObject obj;
            obj.insert("hex", hexEncodedTx);
            requestBody.setObject(obj);

            _requestHandler->makePostRequest(url, requestBody, errorHandler, responseHandler);
        });
    });
}

//==============================================================================

auto XSNBlockExplorerHttpClient::estimateSmartFee(unsigned blocksTarget) -> Promise<double>
{
    return Promise<double>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(this, [=] {
            auto errorHandler = [=](int errorCode, const QString& errorResponse) {
                reject(NetworkUtils::ApiErrorException{ errorCode, errorResponse });
            };

            auto responseHandler = [=](const QByteArray& response) {
                auto obj = QJsonDocument::fromJson(response).object();
                resolve(obj.value("feerate").toDouble());
            };

            QVariantMap params;
            params.insert("nBlocks", blocksTarget);

            const QString url = QString("/blocks/estimate-fee");
            _requestHandler->makeGetRequest(url, params, errorHandler, responseHandler);
        });
    });
}

//==============================================================================

auto XSNBlockExplorerHttpClient::getBlockHashByHeight(unsigned blockHeight) -> Promise<QByteArray>
{
    return Promise<QByteArray>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(this, [=] {
            auto errorHandler = [=](int errorCode, const QString& errorResponse) {
                reject(NetworkUtils::ApiErrorException{ errorCode, errorResponse });
            };

            auto responseHandler = [=](const QByteArray& response) { resolve(response); };

            QVariantMap params;
            params["includeFilter"] = true;

            const QString url = QString("/block-headers/%1").arg(blockHeight);
            _requestHandler->makeGetRequest(url, params, errorHandler, responseHandler);
        });
    });
}

//==============================================================================

auto XSNBlockExplorerHttpClient::getRawTransaction(QString transactionHash) -> Promise<QByteArray>
{
    return Promise<QByteArray>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(this, [=] {
            auto errorHandler = [=](int errorCode, const QString& errorResponse) {
                reject(NetworkUtils::ApiErrorException{ errorCode, errorResponse });
            };

            auto responseHandler = [=](const QByteArray& response) { resolve(response); };

            const QString url = QString("/transactions/%1/lite").arg(transactionHash);
            _requestHandler->makeGetRequest(url, {}, errorHandler, responseHandler);
        });
    });
}

//==============================================================================

auto XSNBlockExplorerHttpClient::getRawTxByIndex(int64_t blockNum, uint32_t txIndex)
    -> Promise<QByteArray>
{
    return Promise<QByteArray>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(this, [=] {
            auto errorHandler = [=](int errorCode, const QString& errorResponse) {
                reject(NetworkUtils::ApiErrorException{ errorCode, errorResponse });
            };

            auto responseHandler = [=](const QByteArray& response) { resolve(response); };

            const QString url
                = QString("/v2/blocks/%1/transactions/%2/lite").arg(blockNum).arg(txIndex);
            _requestHandler->makeGetRequest(url, {}, errorHandler, responseHandler);
        });
    });
}

//==============================================================================

auto XSNBlockExplorerHttpClient::getBlockFilter(QString blockHash) -> Promise<QByteArray>
{
    return Promise<QByteArray>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(this, [=] {
            auto errorHandler = [=](int errorCode, const QString& errorResponse) {
                reject(NetworkUtils::ApiErrorException{ errorCode, errorResponse });
            };

            auto responseHandler = [=](const QByteArray& response) { resolve(response); };

            QVariantMap params;
            params["includeFilter"] = true;

            const QString url = QString("/block-headers/%1").arg(blockHash);
            _requestHandler->makeGetRequest(url, params, errorHandler, responseHandler);
        });
    });
}

//==============================================================================

auto XSNBlockExplorerHttpClient::getTxOut(QString txid, unsigned int outputIndex)
    -> Promise<QByteArray>
{
    return Promise<QByteArray>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(this, [=] {
            auto errorHandler = [=](int errorCode, const QString& errorResponse) {
                reject(NetworkUtils::ApiErrorException{ errorCode, errorResponse });
            };

            auto responseHandler = [=](const QByteArray& response) { resolve(response); };

            const QString url = QString("/transactions/%1/utxos/%2").arg(txid).arg(outputIndex);
            _requestHandler->makeGetRequest(url, {}, errorHandler, responseHandler);
        });
    });
}

//==============================================================================

auto XSNBlockExplorerHttpClient::getSpendingTx(QString txHash, unsigned int outputIndex)
    -> Promise<QByteArray>
{
    return Promise<QByteArray>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(this, [=] {
            auto errorHandler = [=](int errorCode, const QString& errorResponse) {
                reject(NetworkUtils::ApiErrorException{ errorCode, errorResponse });
            };

            auto responseHandler = [=](const QByteArray& response) { resolve(response); };

            const QString url = QString("/transactions/%1/utxos/%2/spending-transaction")
                                    .arg(txHash)
                                    .arg(outputIndex);
            _requestHandler->makeGetRequest(url, {}, errorHandler, responseHandler);
        });
    });
}

//==============================================================================
