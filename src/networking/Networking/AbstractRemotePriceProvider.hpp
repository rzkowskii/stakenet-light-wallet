#ifndef ABSTRACTREMOTEPRICEPROVIDER_HPP
#define ABSTRACTREMOTEPRICEPROVIDER_HPP

#include <QObject>
#include <QString>
#include <QtPromise>

class AbstractRemotePriceProvider : public QObject {
    Q_OBJECT
public:
    explicit AbstractRemotePriceProvider(QObject* parent = nullptr);
    virtual ~AbstractRemotePriceProvider();
    virtual QtPromise::QPromise<double> fetchPrice(QString currencySymbol, QString currency) const = 0;
};

#endif // ABSTRACTREMOTEPRICEPROVIDER_HPP
