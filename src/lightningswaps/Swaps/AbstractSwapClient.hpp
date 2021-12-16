#ifndef ABSTRACTSWAPSCLIENT_HPP
#define ABSTRACTSWAPSCLIENT_HPP

#include <LndTools/LndTypes.hpp>
#include <QObject>
#include <Swaps/Types.hpp>
#include <Utils/Utils.hpp>

namespace swaps {

//==============================================================================

class AbstractSwapClient : public QObject {
    Q_OBJECT
public:
    enum class State { Connected, Disabled, Disconnected, WaitingUnlock, NotInitialized };

    struct Route {
        struct Hop {
            uint64_t chan_id{ 0 };
            int64_t chan_capacity{ 0 };
            int64_t fee{ 0 };
            uint32_t expiry{ 0 };
            int64_t amt_to_forward_msat{ 0 };
            int64_t fee_msat{ 0 };
            std::string pub_key;
        };

        std::vector<Hop> hops;
        uint32_t total_time_lock{ 0 };
        int64_t total_fees_msat{ 0 };
        int64_t total_amt_msat{ 0 };
    };

    using Routes = std::vector<Route>;

    explicit AbstractSwapClient(QObject* parent = nullptr);
    ~AbstractSwapClient();

    /**
     * Verifies that the swap client can be reached and is in an operational state
     * and sets the [[ClientStatus]] accordingly.
     */
    //    protected abstract async verifyConnection(): Promise<void>;

    virtual Promise<void> verifyConnection() = 0;

    /**
     * Sends payment according to the terms of a swap deal.
     * @returns the preimage for the swap
     */
    virtual Promise<std::string> sendPayment(SwapDeal deal) = 0;

    /**
     * @param units the amount of the invoice denominated in the smallest units supported by its
     * currency
     * @returns encoded payment request
     */
    virtual Promise<std::string> addHodlInvoice(
        std::string rHash, u256 units, uint32_t cltvExpiry)
        = 0;
    virtual Promise<void> settleInvoice(std::string rHash, std::string rPreimage, QVariantMap payload) = 0;
    virtual Promise<void> removeInvoice(std::string rHash) = 0;
    virtual Promise<Routes> getRoutes(
        u256 units, std::string destination, std::string currency, uint32_t finalCltvDelta)
        = 0;
    virtual Promise<std::string> destination() const = 0;

    /**
     * Gets the block height of the chain backing this swap client.
     */
    virtual Promise<uint32_t> getHeight() = 0;

    virtual uint32_t finalLock() const = 0;
    virtual ClientType type() const = 0;
    virtual double minutesPerBlock() const = 0;
    virtual uint32_t lockBuffer() const = 0;
    virtual bool isConnected() const = 0;

signals:
    void stateChanged(State newState);
    void htlcAccepted(std::string rHash, int64_t amount);

public slots:

private:
};

//==============================================================================
}

#endif // ABSTRACTSWAPSCLIENT_HPP
