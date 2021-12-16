#include "OwnOrdersHistoryListModel.hpp"
#include <Service/RefundableFeeManager.hpp>
#include <Swaps/AbstractSwapRepository.hpp>
#include <Utils/Utils.hpp>
#include <Tools/Common.hpp>

#include <variant>

//==============================================================================

struct OwnOrdersHistoryListModel::Impl {
    using Data = std::variant<swaps::SwapDeal, storage::RefundableFee>;
    std::vector<Data> data;
};

//==============================================================================

OwnOrdersHistoryListModel::OwnOrdersHistoryListModel(swaps::AbstractSwapRepository* repostiry,
    swaps::RefundableFeeManager* feeManager, QObject* parent)
    : QAbstractListModel(parent)
    , _impl(new Impl)
    , _repostiry(repostiry)
    , _feeManager(feeManager)
{
    init();
}

//==============================================================================

OwnOrdersHistoryListModel::~OwnOrdersHistoryListModel() {}

//==============================================================================

int OwnOrdersHistoryListModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return static_cast<int>(_impl->data.size());
}

//==============================================================================

QVariant OwnOrdersHistoryListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount(QModelIndex())) {
        return QVariant();
    }

    auto& data = _impl->data.at(static_cast<size_t>(index.row()));

    struct Visitor {
        Visitor(int role)
            : role(role)
        {
        }
        QVariant operator()(const swaps::SwapDeal& order) const
        {
            switch (role) {
            case IdRole:
                return QString("type_order/%1").arg(QString::fromStdString(order.orderId));
            case PriceRole:
                return static_cast<double>(order.price);
            case AmountRole:
                return static_cast<double>(order.quantity.get());
            case PairIdRole:
                return QString::fromStdString(order.pairId).replace("_", "/");
            case TimeRole:
                return QDateTime::fromMSecsSinceEpoch(order.executeTime.get())
                    .toString("yyyy-MM-dd HH:mm:ss");
                // TODO: find issue why isBuy is true for both sides
            case IsBuyRole:
                return (order.role == swaps::SwapRole::Maker) ? order.isBuy : !order.isBuy;
            case TotalRole:
                return (order.price / static_cast<double>(COIN)) * order.quantity.get();
            case TypeRole:
                return static_cast<int>(Type::Deal);
            default:
                break;
            }

            return QVariant();
        }
        QVariant operator()(const storage::RefundableFee& fee) const
        {
            switch (role) {
            case IdRole:
                return QString("type_fee/%1").arg(fee.id());
            case PriceRole:
                return static_cast<double>(fee.initialamount());
            case TotalRole:
                return static_cast<double>(fee.initialamount());
            case AmountRole:
                return static_cast<double>(fee.initialamount());
            case PairIdRole:
                return QString::fromStdString(fee.pairid()).replace("_", "/");
            case TimeRole:
                return QDateTime::fromSecsSinceEpoch(fee.timestamp())
                    .toString("yyyy-MM-dd HH:mm:ss");
            case TypeRole:
                return static_cast<int>(Type::Fee);
            case IsBuyRole:
                return fee.is_buy();
            default:
                break;
            }

            return QVariant();
        }

        int role;
    };

    return std::visit(Visitor(role), data);
}

//==============================================================================

QHash<int, QByteArray> OwnOrdersHistoryListModel::roleNames() const
{
    static QHash<int, QByteArray> roles;

    if (roles.empty()) {
        roles[IdRole] = "id";
        roles[AmountRole] = "amount";
        roles[PriceRole] = "price";
        roles[TotalRole] = "total";
        roles[TimeRole] = "time";
        roles[PairIdRole] = "pairId";
        roles[IsBuyRole] = "isBuy";
        roles[TypeRole] = "type";
    }

    return roles;
}

//==============================================================================

void OwnOrdersHistoryListModel::init()
{
    auto onAddDeal = [this](auto deal) {
        if (deal.phase == swaps::SwapPhase::PaymentReceived) {
            this->beginInsertRows({}, this->rowCount(), this->rowCount());
            _impl->data.emplace_back(deal);
            this->endInsertRows();
        }
    };

    auto onFeeAdded = [this](auto fee) {
        this->beginInsertRows({}, this->rowCount(), this->rowCount());
        _impl->data.emplace_back(fee);
        this->endInsertRows();
    };

    auto onFeeBurnt = [this](auto id) {
        auto it
            = std::find_if(std::begin(_impl->data), std::end(_impl->data), [id](const auto& var) {
                  if (auto fee = std::get_if<storage::RefundableFee>(&var)) {
                      return fee->id() == id;
                  } else {
                      return false;
                  }
              });

        if (it != std::end(_impl->data)) {
            auto row = std::distance(std::begin(_impl->data), it);
            this->beginRemoveRows(QModelIndex(), row, row);
            _impl->data.erase(it);
            this->endRemoveRows();
        }
    };

    connect(_repostiry, &swaps::AbstractSwapRepository::dealAdded, this, onAddDeal);
    connect(_repostiry, &swaps::AbstractSwapRepository::dealChanged, this, onAddDeal);

    connect(_feeManager, &swaps::RefundableFeeManager::refundableFeeAdded, this, onFeeAdded);
    connect(_feeManager, &swaps::RefundableFeeManager::refundableFeeBurnt, this, onFeeBurnt);

    using PromiseResult
        = std::tuple<std::vector<swaps::SwapDeal>, std::vector<storage::RefundableFee>>;

    auto promise = Promise<PromiseResult>(
        [rep = _repostiry, feeManager = _feeManager](const auto& resolve, const auto&) {
            QMetaObject::invokeMethod(rep, [=] {
                std::vector<swaps::SwapDeal> result;
                for (auto&& deal : rep->deals()) {
                    if (deal.phase == swaps::SwapPhase::SwapCompleted
                        || deal.phase == swaps::SwapPhase::PaymentReceived) {
                        result.emplace_back(deal);
                    }
                }

                resolve(PromiseResult{ result, feeManager->state()->fees() });
            });
        });

    promise.then([this](PromiseResult values) {
        auto cpy = [this](const auto& from) {
            std::copy(std::begin(from), std::end(from), std::back_inserter(_impl->data));
        };

        this->beginResetModel();

        _impl->data.clear();
        _impl->data.reserve(std::get<0>(values).size() + std::get<1>(values).size());

        cpy(std::get<0>(values));
        cpy(std::get<1>(values));

        this->endResetModel();
    });
}

//==============================================================================
