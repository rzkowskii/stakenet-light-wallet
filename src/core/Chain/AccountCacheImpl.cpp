#include "AccountCacheImpl.hpp"
#include <Data/WalletAssetsModel.hpp>
#include <Tools/DBUtils.hpp>
#include <Utils/GenericProtoDatabase.hpp>
#include <Utils/Logging.hpp>
#include <txdb.h>

//==============================================================================

static const std::string DB_ACCOUNTS_INDEX{ "accounts_cache" };
static const std::string DB_ASSET_INDEX{ "asset_cache" };
static const std::string DB_TOKEN_INDEX{ "token_cache" };


//==============================================================================

AssetAccountCache::AssetAccountCache(QString tokenAddress, Balance balance, Balance nonce, Balance updateHeight, QObject* parent)
    : AbstractAssetAccount(tokenAddress, parent)
    , _executionContext(parent)
    , _balance(balance)
    , _nonce(nonce)
    , _updateHeight(updateHeight)
{
}

//==============================================================================

void AssetAccountCache::setAccountBalance(Balance newBalance)
{
    if (_balance != newBalance) {
        _balance = newBalance;
        balanceChanged(_balance);
        accountChanged();
    }
}

//==============================================================================

void AssetAccountCache::setAccountNonce(Balance newNonce)
{
    if (_nonce != newNonce) {
        _nonce = newNonce;
        nonceChanged(_nonce);
        accountChanged();
    }
}

//==============================================================================

void AssetAccountCache::setUpdateHeight(Balance newUpdateHeight)
{
    if (_updateHeight != newUpdateHeight) {
        _updateHeight = newUpdateHeight;
        updateHeightChanged(_updateHeight);
        accountChanged();
    }
}

//==============================================================================

Promise<Balance> AssetAccountCache::balance() const
{
    return Promise<Balance>([this](const auto& resolver, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] { resolver(_balance); });
    });
}

//==============================================================================

Promise<Balance> AssetAccountCache::nonce() const
{
    return Promise<Balance>([this](const auto& resolver, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] { resolver(_nonce); });
    });
}

//==============================================================================

Balance AssetAccountCache::balanceSync() const
{
    Q_ASSERT_X(thread() == QThread::currentThread(), __FUNCTION__,
        "Calling sync method from different thread");
    return _balance;
}

//==============================================================================

Balance AssetAccountCache::nonceSync() const
{
    Q_ASSERT_X(thread() == QThread::currentThread(), __FUNCTION__,
        "Calling sync method from different thread");
    return _nonce;
}

//==============================================================================

Balance AssetAccountCache::updateHeightSync() const
{
    Q_ASSERT_X(thread() == QThread::currentThread(), __FUNCTION__,
        "Calling sync method from different thread");
    return _updateHeight;
}

//==============================================================================

AccountCacheImpl::AccountCacheImpl(std::shared_ptr<Utils::LevelDBSharedDatabase> provider,
    const WalletAssetsModel& assetsModel, QObject* parent)
    : AssetsAccountsCache(parent)
    , _dbProvider(provider)
    , _assetsModel(assetsModel)
{
}

//==============================================================================

Promise<void> AccountCacheImpl::load()
{
    return Promise<void>([=](const auto& resolver, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            try {
                this->executeLoad();
                resolver();
            } catch (std::exception& ex) {
                LogCCritical(Chains) << "Failed to load asset cache:" << ex.what();
                reject(ex);
            }
        });
    });
}

//==============================================================================

AbstractAssetAccount& AccountCacheImpl::cacheByIdSync(AssetID assetId)
{
    Q_ASSERT_X(thread() == QThread::currentThread(), __FUNCTION__,
        "Calling sync method from different thread");
    return *_caches.at(assetId);
}

//==============================================================================

AbstractMutableAccount& AccountCacheImpl::mutableCacheByIdSync(AssetID assetId)
{
    Q_ASSERT_X(thread() == QThread::currentThread(), __FUNCTION__,
        "Calling sync method from different thread");
    return *_caches.at(assetId);
}

//==============================================================================

std::vector<AssetID> AccountCacheImpl::availableCaches() const
{
    std::vector<AssetID> ids;
    for (const auto [key, _] : _caches) {
        ids.emplace_back(key);
    }
    return ids;
}

//==============================================================================

void AccountCacheImpl::executeLoad()
{
    if (!_loaded) {

        _dbProvider->registerIndex(DB_ACCOUNTS_INDEX, DB_ACCOUNTS_INDEX);
        using TxDB = Utils::GenericProtoDatabase<chain::EthAccount>;
        TxDB::Cache cache;

        auto load = [this](std::string internalIndex,
                        std::function<void(AssetID, const std::vector<unsigned char>&)> onRead) {
            using namespace bitcoin;
            std::unique_ptr<CDBIterator> pcursor(_dbProvider->NewIterator());
            std::pair<std::string, std::pair<std::string, uint32_t>> key{ DB_ACCOUNTS_INDEX,
                std::make_pair(internalIndex, 0) };
            pcursor->Seek(key);

            while (pcursor->Valid() && pcursor->GetKey(key) && key.first == DB_ACCOUNTS_INDEX
                && key.second.first == internalIndex) {
                std::vector<unsigned char> serialization;
                if (pcursor->GetValue(serialization)) {
                    onRead(key.second.second, serialization);
                    pcursor->Next();
                }
            }
        };

        load(DB_ASSET_INDEX, [this](AssetID assetID, const auto& serialization) {
            chain::EthAccount acc;
            if (acc.ParseFromArray(serialization.data(), serialization.size())) {
                auto address = acc.address();
                auto nonce = QString::fromStdString(acc.nonce());
                auto balance = QString::fromStdString(acc.balance());

                _caches.emplace(assetID,
                    new AssetAccountCache(QString {}, balance.toUInt(nullptr, 16),
                        nonce.toUInt(nullptr, 16), 0, this));
                cacheAdded(assetID);
            }
        });

        load(DB_TOKEN_INDEX, [this](AssetID assetID, const auto& serialization) {
            chain::EthToken token;
            if (token.ParseFromArray(serialization.data(), serialization.size())) {
                auto tokenAddress = QString::fromStdString(token.token_address());
                auto balance = QString::fromStdString(token.balance());
                auto updateHeight = token.update_height();

                _caches.emplace(assetID,
                    new AssetAccountCache(tokenAddress, balance.toUInt(nullptr, 16),
                        0, updateHeight, this));
                cacheAdded(assetID);
            }

        });

        for (const auto& assetID : _assetsModel.activeAssets()) {
            auto asset = _assetsModel.assetById(assetID);
            if (asset.type() == CoinAsset::Type::Account) {
                _caches.emplace(asset.coinID(), new AssetAccountCache(QString{}, 0, 0, 0, this));
                cacheAdded(asset.coinID());
            }
        }

        for (auto [assetID, cache] : _caches) {
            connect(cache, &AssetAccountCache::accountChanged, this,
                [id = assetID, this] { executeSaveAccount(id); }, Qt::DirectConnection);
        }

        _loaded = true;
        LogCDebug(Chains) << "AccountCache loaded";
    }
}

//==============================================================================

AssetAccountCache& AccountCacheImpl::getOrCreateCache(AssetID assetID)
{
    if (_caches.count(assetID) == 0) {
    }

    return *_caches.at(assetID);
}

//==============================================================================

void AccountCacheImpl::executeSaveAccount(AssetID assetID)
{
    Q_ASSERT_X(thread() == QThread::currentThread(), __FUNCTION__,
        "Calling sync method from different thread");

    if (_caches.count(assetID) == 0) {
        return;
    }

    const auto& cache = *_caches.at(assetID);

    auto asset = _assetsModel.assetById(assetID);
    auto balance = QString::number(cache.balanceSync(), 16).toStdString();
    if (asset.token()) {
        chain::EthToken token;
        token.set_balance(balance);
        token.set_token_address(cache.tokenAddress().toStdString());
        token.set_update_height(cache.updateHeightSync());
        _dbProvider->Write(std::make_pair(DB_ACCOUNTS_INDEX, std::make_pair(DB_TOKEN_INDEX, assetID)), token.SerializeAsString());
    } else {
        chain::EthAccount acc;
//        acc.address()
        acc.set_balance(balance);
        acc.set_nonce(QString::number(cache.nonceSync(), 16).toStdString());
        _dbProvider->Write(std::make_pair(DB_ACCOUNTS_INDEX, std::make_pair(DB_ASSET_INDEX ,assetID)), acc.SerializeAsString());
    }
}

//==============================================================================
