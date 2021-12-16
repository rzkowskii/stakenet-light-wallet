#ifndef ACCOUNTCACHEIMPL_HPP
#define ACCOUNTCACHEIMPL_HPP

#include <Chain/AbstractAssetAccount.hpp>
#include <Chain/Protos/Chain.pb.h>
#include <QObject>

class WalletAssetsModel;

namespace Utils {
class LevelDBSharedDatabase;
}

//==============================================================================

class AssetAccountCache : public AbstractAssetAccount, public AbstractMutableAccount {
    Q_OBJECT
public:
    explicit AssetAccountCache(
        QString tokenAddress, Balance balance, Balance nonce, Balance updateHeight, QObject* parent = nullptr);

    // AbstractMutableAccount interface
    void setAccountBalance(Balance newBalance) override;
    void setAccountNonce(Balance newNonce) override;
    void setUpdateHeight(Balance newUpdateHeight) override;
    Promise<Balance> balance() const override;
    Promise<Balance> nonce() const override;
    Balance balanceSync() const override;
    Balance nonceSync() const override;
    Balance updateHeightSync() const override;

signals:
    void accountChanged();

private:
    QObject* _executionContext{ nullptr };
    Balance _balance{ 0 };
    Balance _nonce{ 0 };
    Balance _updateHeight{ 0 };
};

//==============================================================================

class AccountCacheImpl : public AssetsAccountsCache {
    Q_OBJECT
public:
    explicit AccountCacheImpl(std::shared_ptr<Utils::LevelDBSharedDatabase> provider,
        const WalletAssetsModel& assetsModel, QObject* parent = nullptr);

    // AssetsAccountsCache interface
public:
    Promise<void> load() override;
    AbstractAssetAccount& cacheByIdSync(AssetID assetId) override;
    AbstractMutableAccount& mutableCacheByIdSync(AssetID assetId) override;
    std::vector<AssetID> availableCaches() const override;

private:
    void executeLoad();
    //    void executeSaveTxns(AssetID assetID, const std::vector<Transaction>& txns) const;
    AssetAccountCache& getOrCreateCache(AssetID assetID);
    void executeSaveAccount(AssetID assetID);

private:
    std::shared_ptr<Utils::LevelDBSharedDatabase> _dbProvider;
    const WalletAssetsModel& _assetsModel;
    mutable std::unordered_map<AssetID, AssetAccountCache*> _caches;
    std::atomic_bool _loaded{ false };
};

//==============================================================================

#endif // ACCOUNTCACHEIMPL_HPP
