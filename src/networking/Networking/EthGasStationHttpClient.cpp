#include "EthGasStationHttpClient.hpp"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>

//==============================================================================

EthGasStationHttpClient::EthGasStationHttpClient(
    QNetworkAccessManager* accessManager, QObject* parent)
    : AbstractEthGasProvider(parent)
    , _accessManager(accessManager)
{
}

//==============================================================================

Promise<AbstractEthGasProvider::GasPrices> EthGasStationHttpClient::fetchGasPrice()
{
    QUrl url("https://ethgasstation.info/api/ethgasAPI.json");
    QUrlQuery query;
    query.addQueryItem("api-key", "9e444d173ce53652fb4925171aec6938c0d7840df5d1abffe8ca553fbd8a");
    url.setQuery(query);
    QNetworkRequest req(url);
    return Promise<QByteArray>([this, req](const auto& resolve, const auto& reject) {
        auto reply = _accessManager->get(req);
        QtPromise::connect(reply, &QNetworkReply::finished)
            .then([reply, reject, resolve] {
                if (reply->error() != QNetworkReply::NoError) {
                    reject(std::runtime_error(reply->errorString().toStdString()));
                } else {
                    resolve(reply->readAll());
                }
            })
            .finally([reply] { reply->deleteLater(); });
    }).then([](QByteArray json) {
        auto rootObject = QJsonDocument::fromJson(json).object();
        //
        auto transform = [](QJsonValue val) {
            // here we receive a value that is / 10 gwei, so we need to divide by 10 and
            // convert to gwei
            auto value = val.toInt() / 10;
            return eth::shannon * value;
        };
        GasPrices prices;
        prices.slow = transform(rootObject.value("safeLow"));
        prices.standard = transform(rootObject.value("average"));
        prices.fast = transform(rootObject.value("fast"));
        return prices;
    });
}

//==============================================================================
