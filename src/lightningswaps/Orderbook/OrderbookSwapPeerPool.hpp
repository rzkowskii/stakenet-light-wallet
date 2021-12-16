#ifndef ORDERBOOKSWAPPEERPOOL_HPP
#define ORDERBOOKSWAPPEERPOOL_HPP

#include <QObject>
#include <memory>
#include <unordered_map>

#include <Orderbook/OrderbookEventDispatcher.hpp>
#include <Swaps/AbstractSwapPeerPool.hpp>

namespace orderbook {

class OrderbookSwapPeer;
class OrderbookClient;

class OrderbookSwapPeerPool : public swaps::AbstractSwapPeerPool {
    Q_OBJECT
public:
    explicit OrderbookSwapPeerPool(OrderbookClient& client, QObject* parent = nullptr);
    ~OrderbookSwapPeerPool() override;

    swaps::AbstractSwapPeer* peerByPubKey(std::string pubKey) const override;

    void addSwapPeer(std::string pubKey);

signals:

public slots:

private slots:
    void onPeerMessageReceived(Event::ServerEvent event);

private:
    OrderbookSwapPeer* getPeerByPubKey(std::string pubKey) const;
    void handlePeerMessage(OrderbookSwapPeer* peer, std::string serialized);

private:
    OrderbookClient& _client;
    std::unordered_map<std::string, std::unique_ptr<OrderbookSwapPeer>> _connectedPeers;
    std::unordered_map<std::string, int> _lastSwapState;
};
}

#endif // ORDERBOOKSWAPPEERPOOL_HPP
