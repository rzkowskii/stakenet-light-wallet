#ifndef REFUNDABLEFEEMANAGERSTATE_HPP
#define REFUNDABLEFEEMANAGERSTATE_HPP

#include <Swaps/Protos/Storage.pb.h>
#include <Utils/Utils.hpp>

#include <QObject>
#include <unordered_map>

namespace Utils {
template <class T> class GenericProtoDatabase;
}

namespace swaps {

class AbstractRefundableFeeRepository;

class RefundableFeeManagerState : public QObject {
    Q_OBJECT
public:
    using FeeRepostiry = Utils::GenericProtoDatabase<storage::RefundableFee>;
    struct RefundableAmount {
        int64_t pending{ 0 };
        int64_t available{ 0 };

        bool operator==(const RefundableAmount& rhs) const;
        bool operator!=(const RefundableAmount& rhs) const;
    };

    explicit RefundableFeeManagerState(const FeeRepostiry& feeRepostiry, QObject* parent = nullptr);
    Promise<RefundableAmount> refundableAmountByCurrency(std::string currency);
    void refresh(std::string currency = {});

    static bool IsRefundable(storage::RefundableFee fee);

    std::vector<storage::RefundableFee> fees() const;

signals:
    void refundableAmountChanged(std::string currency, RefundableAmount amount);

private slots:
    void onRefreshTimeout();

private:
    void init();
    void updateFeesCache(const std::unordered_map<std::string, RefundableAmount>& newAmounts);

private:
    const FeeRepostiry& _repository;
    std::unordered_map<std::string, RefundableAmount> _refundableAmountCache;
};
}

#endif // REFUNDABLEFEEMANAGERSTATE_HPP
