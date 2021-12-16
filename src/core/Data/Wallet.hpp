#ifndef WALLET_HPP
#define WALLET_HPP

#include <Chain/BlockFilterMatcher.hpp>
#include <Data/TransactionEntry.hpp>
#include <Models/AbstractKeychain.hpp>
#include <Models/WalletDataSource.hpp>
#include <QPointer>
#include <QVector>
#include <atomic>
#include <map>
#include <memory>
#include <set>
#include <transaction.h>

namespace bitcoin {
class CTxDataBase;
class CWallet;
class CMnemonic;
class CKeyID;
struct CChainParams;
class CHDChain;
class COutPoint;
class CTxOutDB;
class uint256;
class ECCVerifyHandle;
class CTransactionEntryDB;
namespace interfaces {
    class Chain;
    struct CoinsView;
    struct ReserveKeySource;
}
}

class AbstractMutableChainManager;
class AssetsTransactionsCache;
class WalletAssetsModel;

class UTXOSet {
public:
    void addUnspentUTXO(AssetID assetID, bitcoin::COutPoint prevInput, bitcoin::CTxOut output);
    void spendUTXO(AssetID assetID, bitcoin::COutPoint outpoint, QString spendingTransactionId);
    bool removeUnspentUTXO(AssetID assetID, bitcoin::COutPoint outpoint);
    void lockOutpoint(AssetID assetID, bitcoin::COutPoint outpoint);
    void unlockOutpoint(AssetID assetID, bitcoin::COutPoint outpoint);

    // UTXO and if coin is unspent
    bool hasCoin(AssetID assetID, const bitcoin::COutPoint& outpoint) const;
    bool isSpent(AssetID assetID, const bitcoin::COutPoint& outpoint) const;
    bool isLocked(AssetID assetID, const bitcoin::COutPoint& outpoint) const;
    bitcoin::CTxOut accessCoin(AssetID assetID, const bitcoin::COutPoint& outpoint) const;
    QString outpointSpentIn(AssetID assetID, const bitcoin::COutPoint& outpoint) const;

    std::vector<bitcoin::COutPoint> allUnspentCoins(AssetID assetID) const;

private:
    std::map<AssetID, std::map<bitcoin::COutPoint, bitcoin::CTxOut>> _utxoSet;
    std::map<AssetID, std::map<bitcoin::COutPoint, QString>> _spentUTXOSet;
    std::map<AssetID, std::set<bitcoin::COutPoint>> _lockedOutpoints;
};

class Wallet : public WalletDataSource,
               public BlockFilterMatchable,
               public UTXOSetDataSource,
               public AbstractKeychain,
               public AccountDataSource {
    Q_OBJECT
public:
    explicit Wallet(const WalletAssetsModel& assetsModel,
        AssetsTransactionsCache& transactionsCache, const AbstractMutableChainManager& chainManager,
        bool emulated, QObject* parent = nullptr);

    ~Wallet() override;

    Promise<void> createWalletWithMnemonic() override;
    Promise<void> loadWallet(std::optional<SecureString> password) override;
    Promise<void> encryptWallet(SecureString password) override;
    Promise<MutableTransactionRef> createSendTransaction(bitcoin::UTXOSendTxParams params) override;
    Promise<void> restoreWalletWithMnemAndPass(QString mnemonic) override;
    Promise<QString> getReceivingAddress(AssetID id, Enums::AddressType addressType) const override;
    Promise<QString> getChangeAddress(AssetID id, Enums::AddressType addressType) const override;
    Promise<QString> getMnemonic() const override;
    Promise<AddressesList> getAllKnownAddressesById(AssetID assetId) const override;
    Promise<void> setAddressType(AssetID assetID, Enums::AddressType addressType) override;
    Promise<Enums::AddressType> getAddressType(AssetID assetID) const override;
    Enums::AddressType getAddressTypeSync(AssetID assetID) const override;
    Promise<QByteArray> dumpPrivKey(
        AssetID assetID, std::vector<unsigned char> scriptPubKey) const override;
    Promise<bool> isOurAddress(
        AssetID assetID, std::vector<unsigned char> scriptPubKey) const override;

    OnChainTxRef applyTransaction(OnChainTxRef source, LookupTxById lookupTx) override;
    Promise<OnChainTxRef> applyTransactionAsync(OnChainTxRef source) override;
    void undoTransaction(OnChainTxRef transaction) override;
    BFMatcherUniqueRef createMatcher(AssetID assetID) const override;

    bool isCreated() const override;
    bool isEncrypted() const override;
    bool isLoaded() const override;
    bool isEmpty(AssetID assetID) const override;

    QString identityPubKey() const override;

    // UTXOSetDataSource interface
    Promise<boost::optional<Wire::TxOut>> getUTXO(
        AssetID assetID, const Wire::OutPoint& outpoint) const override;
    Promise<std::vector<std::tuple<Wire::OutPoint, Wire::TxOut>>> listUTXOs(
        AssetID assetID) const override;
    Promise<void> lockOutpoint(AssetID assetID, Wire::OutPoint outpoint) override;
    Promise<void> unlockOutpoint(AssetID assetID, Wire::OutPoint outpoint) override;
    Promise<void> load() override;

    // AbstractKeychain interface
public:
    Promise<KeyDescriptor> deriveNextKey(AssetID assetID, uint32_t keyFamily) override;
    Promise<KeyDescriptor> deriveKey(AssetID assetID, KeyLocator locator) override;
    Promise<std::string> derivePrivKey(AssetID assetID, KeyDescriptor descriptor) override;

    // AccountDataSource interface
public:
    Promise<QString> dumpPrivKey(AssetID id) const override;
    Promise<QString> getAccountAddress(AssetID id) const override;
    Promise<eth::SignedTransaction> createSendTransaction(eth::AccountSendTxParams params) override;

private:
    void init();
    void initHdAccounts(bitcoin::CWallet& wallet) const;
    void initAddressTypes(bitcoin::CWallet& wallet) const;
    void recoverAuxChain(bitcoin::CWallet& wallet) const;
    void loadUtxoSet();
    void loadAddressType();
    void setIsWalletEncrypted(bool isEncrypted);
    // can only be used once cause CWallet creates inside (BerkeleyDatabase inserted.second
    // failed)
    void updateIsWalletEncrypted();

    struct Interfaces {
        std::unique_ptr<bitcoin::interfaces::Chain> chain;
        std::unique_ptr<bitcoin::interfaces::CoinsView> coinsView;
        std::unique_ptr<bitcoin::interfaces::ReserveKeySource> keySource;
    };

    Interfaces createInterfaces(AssetID assetID, size_t bestHeight) const;
    void maintainUTXOSet(const OnChainTx& transaction);
    void applyTransactionHelper(const OnChainTx& filteredTx);
    void markAddressAsUsed(AssetID id, QString address);
    OnChainTxRef tryApplyTransactionHelper(
        OnChainTxRef source, LookupTxById extraLookupTx = {}) const;

private:
    QObject* _executionContext{ nullptr };
    const WalletAssetsModel& _assetsModel;
    std::unique_ptr<bitcoin::ECCVerifyHandle> _secp256k1VerifyHandle;
    std::unique_ptr<bitcoin::CWallet> _wallet;
    AssetsTransactionsCache& _transactionsCache;
    const AbstractMutableChainManager& _chainManager;
    std::atomic_bool _isLoaded{ false };
    std::atomic_bool _isCreated{ false };
    std::atomic_bool _isUtxoLoaded{ false };
    std::vector<AssetID> _notEmptyAssets;
    UTXOSet _utxoSet;
    std::map<AssetID, std::set<QString>> _transactionsApplied;
    bool _emulated{ false };
    bool _isEncrypted{ false };
};

#endif // WALLET_HPP
