#ifndef CONNEXTRESOLVEPAYMENTPROXY_HPP
#define CONNEXTRESOLVEPAYMENTPROXY_HPP

#include <QObject>
#include <Swaps/Types.hpp>

namespace swaps {

class ConnextResolvePaymentProxy : public QObject {
    Q_OBJECT
public:
    explicit ConnextResolvePaymentProxy(std::string rHash, QObject* parent = nullptr);
    void onResolveRequest(ResolvedTransferResponse details);

signals:
    void resolved(std::string rPreimage);

private:
    std::string _rHash;
};
}

#endif // CONNEXTRESOLVEPAYMENTPROXY_HPP
