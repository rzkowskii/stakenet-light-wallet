#include "CMCRemotePriceProvider.hpp"
#include <Networking/RequestHandlerImpl.hpp>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

//==============================================================================

namespace {
static size_t MapAssetID(uint32_t assetID)
{
    static std::map<uint32_t, size_t> ids{ { 0, 1 }, { 2, 2 }, { 384, 2633 } };

    return ids.at(assetID);
}
}

//==============================================================================

CMCRemotePriceProvider::CMCRemotePriceProvider(std::unique_ptr<RequestHandlerImpl>&& requestHandler, QObject* parent)
    : AbstractRemotePriceProvider(parent)
    , _executionContext(new QObject(this))
{
    requestHandler->setParent(this);
    _requestHandler = requestHandler.release();
}

//==============================================================================

QtPromise::QPromise<double> CMCRemotePriceProvider::fetchPrice(
    QString assetSymbol, QString currency) const
{
    return QtPromise::QPromise<double>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            try {
                auto errorHandler = [=](int /*errorCode*/, const QString& errorResponse) {
                    reject(std::runtime_error(errorResponse.toStdString()));
                };

                auto responseHandler = [=](const QByteArray& response) {
                    auto root
                        = QJsonDocument::fromJson(response).object();
                    resolve(root.value(currency).toDouble());
                };

                auto path = QString("/%1/prices").arg(assetSymbol);
                _requestHandler->makeGetRequest(
                    path, { { "currency", currency }  }, errorHandler, responseHandler);

            } catch (std::exception& ex) {
                reject(ex);
            }
        });
    });
}

//==============================================================================
