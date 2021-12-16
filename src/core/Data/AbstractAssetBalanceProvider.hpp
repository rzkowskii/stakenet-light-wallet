#ifndef ABSTRACTASSETBALANCEPROVIDER_HPP
#define ABSTRACTASSETBALANCEPROVIDER_HPP

#include <QObject>
#include <Tools/Common.hpp>
#include <optional>

//==============================================================================

struct AssetBalance {
    Balance balance{ 0 };
    Balance confirmedBalance{ 0 };
    Balance nodeBalance{ 0 };
    Balance availableNodeBalance{ 0 };
    Balance activeNodeBalance{ 0 };

    bool operator==(const AssetBalance& rhs) const;
    bool operator!=(const AssetBalance& rhs) const;
    Balance total() const;
};

//==============================================================================

class AbstractAssetBalanceProvider : public QObject {
    Q_OBJECT
public:
    explicit AbstractAssetBalanceProvider(QObject* parent = nullptr);
    AssetBalance balance() const;

    virtual void update() = 0;

signals:
    void balanceChanged(AssetBalance newBalance);

protected:
    void setBalance(AssetBalance newBalance);

private:
    AssetBalance _balance;
};

//==============================================================================

#endif // ABSTRACTASSETBALANCEPROVIDER_HPP
