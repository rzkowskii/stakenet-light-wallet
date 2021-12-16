#ifndef WALLETDATASOURCE_HPP
#define WALLETDATASOURCE_HPP

#include <QObject>
#include <atomic>
#include <functional>

#include <Chain/BlockHeader.hpp>
#include <Data/SendTransactionParams.hpp>
#include <Data/TransactionEntry.hpp>
#include <EthCore/Encodings.hpp>
#include <Tools/Common.hpp>
#include <Utils/Utils.hpp>
#include <core_types.h>
#include <support/allocators/secure.h>

class Chain;

//==============================================================================

class WalletDataSource : public QObject {
    Q_OBJECT
public:
    explicit WalletDataSource(QObject* parent = nullptr);
    virtual ~WalletDataSource();

    virtual Promise<void> createWalletWithMnemonic() = 0;
    virtual Promise<void> loadWallet(std::optional<SecureString> password) = 0;
    virtual Promise<void> encryptWallet(SecureString password) = 0;
    virtual Promise<MutableTransactionRef> createSendTransaction(bitcoin::UTXOSendTxParams params)
        = 0;
    virtual Promise<void> restoreWalletWithMnemAndPass(QString mnemonic) = 0;

    virtual Promise<QString> getReceivingAddress(
        AssetID id, Enums::AddressType addressType) const = 0;
    virtual Promise<QString> getChangeAddress(AssetID id, Enums::AddressType addressType) const = 0;
    virtual Promise<QString> getMnemonic() const = 0;

    virtual Promise<Enums::AddressType> getAddressType(AssetID assetID) const = 0;
    virtual Enums::AddressType getAddressTypeSync(
        AssetID assetID) const = 0; // call this only from same thread, not thread safe version
    virtual Promise<void> setAddressType(AssetID assetID, Enums::AddressType addressType) = 0;

    virtual Promise<AddressesList> getAllKnownAddressesById(AssetID assetId) const = 0;

    virtual bool isCreated() const = 0;
    virtual bool isLoaded() const = 0;
    virtual bool isEncrypted() const = 0;
    virtual bool isEmpty(AssetID assetID) const = 0;

    virtual QString identityPubKey() const = 0;

    // applies transaction to data source. Effectively marks addresses as used and returns only
    // related inputs/outputs. Can be used with custom lookup function.
    using LookupTxById = std::function<OnChainTxRef(QString)>;
    virtual OnChainTxRef applyTransaction(OnChainTxRef source, LookupTxById lookupTx = {}) = 0;
    virtual Promise<OnChainTxRef> applyTransactionAsync(OnChainTxRef source) = 0;
    // undo transaction changes on UTXO set. Effectively rollbacks utxo set in a state before
    // applying transaction
    virtual void undoTransaction(OnChainTxRef transaction) = 0;

    // TODO(yuraolex): remove this later, only for lnd testing
    virtual Promise<QByteArray> dumpPrivKey(
        AssetID assetID, std::vector<unsigned char> scriptPubKey) const;

    virtual Promise<bool> isOurAddress(
        AssetID assetID, std::vector<unsigned char> scriptPubKey) const = 0;

signals:
    void isWalletCryptedChanged();
};

//==============================================================================

class AccountDataSource {
public:
    virtual ~AccountDataSource();
    // dump hex serializaed private key for primary account address
    virtual Promise<QString> dumpPrivKey(AssetID id) const = 0;
    virtual Promise<QString> getAccountAddress(AssetID id) const = 0;
    virtual Promise<eth::SignedTransaction> createSendTransaction(eth::AccountSendTxParams params)
        = 0;
};

//==============================================================================

class UTXOSetDataSource {
public:
    virtual ~UTXOSetDataSource();
    virtual Promise<boost::optional<Wire::TxOut>> getUTXO(
        AssetID assetID, const Wire::OutPoint& outpoint) const = 0;
    virtual Promise<std::vector<std::tuple<Wire::OutPoint, Wire::TxOut>>> listUTXOs(
        AssetID assetID) const = 0;
    virtual Promise<void> lockOutpoint(AssetID assetID, Wire::OutPoint outpoint) = 0;
    virtual Promise<void> unlockOutpoint(AssetID assetID, Wire::OutPoint outpoint) = 0;
    // loads utxo data set using transactions database
    // should be called only when txdb is initialized
    virtual Promise<void> load() = 0;
};

//==============================================================================

#endif // WALLETDATASOURCE_HPP
