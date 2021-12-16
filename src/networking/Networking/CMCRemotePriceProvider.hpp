#ifndef CMCREMOTEPRICEPROVIDER_HPP
#define CMCREMOTEPRICEPROVIDER_HPP

#include <Networking/AbstractRemotePriceProvider.hpp>
#include <QObject>
#include <memory>

class RequestHandlerImpl;

class CMCRemotePriceProvider : public AbstractRemotePriceProvider {
    Q_OBJECT
public:
    explicit CMCRemotePriceProvider(
        std::unique_ptr<RequestHandlerImpl>&& requestHandler, QObject* parent = nullptr);

    // AbstractRemotePriceProvider interface
public:
    QtPromise::QPromise<double> fetchPrice(QString assetSymbol, QString currency) const override;

private:
    QObject* _executionContext{ nullptr };
    RequestHandlerImpl* _requestHandler{ nullptr };
};

#endif // CMCREMOTEPRICEPROVIDER_HPP
