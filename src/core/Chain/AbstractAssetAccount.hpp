#ifndef ABSTRACTASSETACCOUNT_HPP
#define ABSTRACTASSETACCOUNT_HPP

#include <QObject>
#include <Tools/Common.hpp>
#include <Utils/Utils.hpp>
#include <core_types.h>

//==============================================================================

struct AbstractMutableAccount {
    virtual ~AbstractMutableAccount();

    virtual void setAccountBalance(Balance newBalance) = 0;
    virtual void setAccountNonce(Balance newNonce) = 0;
    virtual void setUpdateHeight(Balance newUpdateHeight) = 0;
};

//==============================================================================

class AbstractAssetAccount : public QObject {
    Q_OBJECT
public:
    explicit AbstractAssetAccount(QString tokenAddress, QObject* parent = nullptr);

    virtual Promise<Balance> balance() const = 0;
    virtual Promise<Balance> nonce() const = 0;

    virtual Balance balanceSync() const = 0;
    virtual Balance nonceSync() const = 0;
    virtual Balance updateHeightSync() const = 0;

    QString tokenAddress() const;

signals:
    void balanceChanged(Balance newBalance);
    void nonceChanged(Balance newNonce);
    void updateHeightChanged(Balance newUpdateHeight);

private:
    QString _tokenAddress;
};

//==============================================================================

class AssetsAccountsCache : public QObject {
    Q_OBJECT
public:
    explicit AssetsAccountsCache(QObject* parent = nullptr);

    virtual Promise<void> load() = 0;

    Promise<AbstractAssetAccount*> cacheById(AssetID assetId);

    virtual AbstractAssetAccount& cacheByIdSync(AssetID assetId) = 0;
    virtual AbstractMutableAccount& mutableCacheByIdSync(AssetID assetId) = 0;
    virtual std::vector<AssetID> availableCaches() const = 0;

signals:
    void cacheAdded(AssetID assetID);

protected:
    QObject* _executionContext{ nullptr };
};

//==============================================================================

#endif // ABSTRACTASSETACCOUNT_HPP
