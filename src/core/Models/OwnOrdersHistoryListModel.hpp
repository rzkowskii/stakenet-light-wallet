#ifndef OWNORDERHISTORYLISTMODEL_HPP
#define OWNORDERHISTORYLISTMODEL_HPP

#include <QAbstractListModel>
#include <QDateTime>
#include <QPointer>
#include <memory>

namespace swaps {
class AbstractSwapRepository;
class RefundableFeeManager;
}

class OwnOrdersHistoryListModel : public QAbstractListModel {
    Q_OBJECT
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")
public:
    enum class Type { Deal, Fee };
    Q_ENUM(Type)

    enum Roles {
        IdRole = Qt::UserRole + 1,
        PriceRole,
        AmountRole,
        PairIdRole,
        TimeRole,
        IsBuyRole,
        TotalRole,
        TypeRole
    };
    Q_ENUMS(Roles)

    OwnOrdersHistoryListModel(swaps::AbstractSwapRepository* repostiry,
        swaps::RefundableFeeManager* feeManager, QObject* parent = nullptr);
    ~OwnOrdersHistoryListModel() override;

    virtual int rowCount(const QModelIndex& parent = {}) const override final;
    virtual QVariant data(const QModelIndex& index, int role) const override final;
    virtual QHash<int, QByteArray> roleNames() const override final;

signals:

public slots:

private:
    void init();

private:
    struct Impl;
    std::unique_ptr<Impl> _impl;
    QPointer<swaps::AbstractSwapRepository> _repostiry;
    QPointer<swaps::RefundableFeeManager> _feeManager;
};

#endif // OWNORDERHISTORYLISTMODEL_HPP
