#ifndef ABSTRACTSWAPREPOSITORY_HPP
#define ABSTRACTSWAPREPOSITORY_HPP

#include <QObject>
#include <Swaps/Types.hpp>

namespace swaps {

class AbstractSwapRepository : public QObject {
    Q_OBJECT
public:
    using Deals = std::vector<SwapDeal>;
    explicit AbstractSwapRepository(QObject* parent = nullptr);

    void load();
    void saveDeal(SwapDeal deal);
    void updateDeal(SwapDeal deal);
    boost::optional<SwapDeal> deal(std::string rHash) const;
    const Deals& deals() const;

signals:
    void dealAdded(SwapDeal deal);
    void dealChanged(SwapDeal deal);

public slots:

protected:
    virtual Deals executeLoadAll() = 0;
    virtual void executeSaveDeal(SwapDeal deal) = 0;

private:
    Deals _deals;
};
}

#endif // ABSTRACTSWAPREPOSITORY_HPP
