#ifndef SWAPPEERPOOL_HPP
#define SWAPPEERPOOL_HPP

#include <QObject>
#include <Swaps/AbstractSwapPeerPool.hpp>

namespace swaps {

class SwapPeerPool : public AbstractSwapPeerPool {
    Q_OBJECT
public:
    explicit SwapPeerPool(QObject* parent = nullptr);
    AbstractSwapPeer* peerByPubKey(std::string pubKey) const override;

signals:

public slots:
};
}

#endif // SWAPPEERPOOL_HPP
