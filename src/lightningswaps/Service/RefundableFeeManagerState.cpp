#include "RefundableFeeManagerState.hpp"
#include <Utils/GenericProtoDatabase.hpp>

#include <QDateTime>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>
#include <mutex>

using namespace boost::adaptors;

namespace swaps {

//==============================================================================

static const constexpr int64_t FEE_LOCK_INTERVAL = 60 * 60 * 24;

//==============================================================================

static std::unordered_map<std::string, RefundableFeeManagerState::RefundableAmount>
CalculateNewCacheValues(const std::vector<storage::RefundableFee>& allFees)
{
    std::unordered_map<std::string, RefundableFeeManagerState::RefundableAmount> amounts;
    for (const auto& fee : allFees) {
        auto& amount = amounts[fee.currency()];
        if (RefundableFeeManagerState::IsRefundable(fee)) {
            amount.available += fee.initialamount();
        } else {
            amount.pending += fee.initialamount();
        }
    }

    return amounts;
}

//==============================================================================

RefundableFeeManagerState::RefundableFeeManagerState(
    const FeeRepostiry& feeRepostiry, QObject* parent)
    : QObject(parent)
    , _repository(feeRepostiry)
{
    init();
}

//==============================================================================

Promise<RefundableFeeManagerState::RefundableAmount>
RefundableFeeManagerState::refundableAmountByCurrency(std::string currency)
{
    return Promise<RefundableAmount>([=](const auto& resolve, const auto& /*reject*/) {
        QMetaObject::invokeMethod(this, [=] {
            if (_refundableAmountCache.count(currency) > 0) {
                resolve(_refundableAmountCache.at(currency));
            } else {
                resolve(RefundableAmount{});
            }
        });
    });
}

//==============================================================================

void RefundableFeeManagerState::refresh(std::string currency)
{
    QMetaObject::invokeMethod(this, [=] {
        if (currency.empty()) {
            onRefreshTimeout();
            return;
        }

        std::vector<storage::RefundableFee> fees;

        auto pred = [currency](const auto& fee) { return fee.second.currency() == currency; };
        auto toVec = [](const auto& it) { return it.second; };
        boost::copy(
            _repository.values() | filtered(pred) | transformed(toVec), std::back_inserter(fees));
        updateFeesCache(CalculateNewCacheValues(fees));
    });
}

//==============================================================================

bool RefundableFeeManagerState::IsRefundable(storage::RefundableFee fee)
{
    auto currentTimestamp = QDateTime::currentSecsSinceEpoch();
    return currentTimestamp - fee.timestamp() > FEE_LOCK_INTERVAL
        && fee.type() == ::storage::RefundableFee_Type::RefundableFee_Type_ORDER_FEE;
}

//==============================================================================

std::vector<storage::RefundableFee> RefundableFeeManagerState::fees() const
{
    std::vector<storage::RefundableFee> result(_repository.values().size());
    std::transform(std::begin(_repository.values()), std::end(_repository.values()),
        std::begin(result), [](const auto& it) { return it.second; });
    return result;
}

//==============================================================================

void RefundableFeeManagerState::onRefreshTimeout()
{
    std::vector<storage::RefundableFee> fees;
    auto toVec = [](const auto& it) { return it.second; };
    boost::copy(_repository.values() | transformed(toVec), std::back_inserter(fees));
    updateFeesCache(CalculateNewCacheValues(fees));
}

//==============================================================================

void RefundableFeeManagerState::init()
{
    static std::once_flag once;
    std::call_once(once, [] { qRegisterMetaType<RefundableAmount>("RefundableAmount"); });

    auto refreshTimer = new QTimer(this);
    connect(refreshTimer, &QTimer::timeout, this, &RefundableFeeManagerState::onRefreshTimeout);
    refreshTimer->setSingleShot(false);
    refreshTimer->setInterval(60 * 1000 * 5); // check every 5 minutes
    refreshTimer->start();
    onRefreshTimeout();
}

//==============================================================================

void RefundableFeeManagerState::updateFeesCache(
    const std::unordered_map<std::string, RefundableFeeManagerState::RefundableAmount>& newAmounts)
{
    for (auto&& it : newAmounts) {
        auto& oldAmount = _refundableAmountCache[it.first];
        if (oldAmount != it.second) {
            oldAmount = it.second;
        }
        refundableAmountChanged(it.first, it.second);
    }
}

//==============================================================================

bool RefundableFeeManagerState::RefundableAmount::operator==(
    const RefundableFeeManagerState::RefundableAmount& rhs) const
{
    return pending == rhs.pending && available == rhs.available;
}

//==============================================================================

bool RefundableFeeManagerState::RefundableAmount::operator!=(
    const RefundableFeeManagerState::RefundableAmount& rhs) const
{
    return !(*this == rhs);
}

//==============================================================================
}
