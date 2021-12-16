#ifndef ABSTRACTORDERBOOKCLIENT_HPP
#define ABSTRACTORDERBOOKCLIENT_HPP

#include <QObject>
#include <boost/optional.hpp>

#include <Orderbook/Types.hpp>

namespace orderbook {

class AbstractOrderbookClient : public QObject {
    Q_OBJECT
public:
    explicit AbstractOrderbookClient(QObject* parent = nullptr);
    virtual OwnOrder getOwnOrder(std::string pairId, std::string orderId) const = 0;

signals:
    void orderAdded();
    void orderRemoved();

public slots:
};
}

#endif // ABSTRACTORDERBOOKCLIENT_HPP
