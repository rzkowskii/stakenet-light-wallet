#ifndef EMULATORWALLETDATASOURCE_HPP
#define EMULATORWALLETDATASOURCE_HPP

#include <Chain/BlockFilterMatcher.hpp>
#include <Data/TransactionEntry.hpp>
#include <Data/WalletAssetsModel.hpp>
#include <Models/AbstractKeychain.hpp>
#include <Models/WalletDataSource.hpp>
#include <QMap>
#include <QObject>
#include <QPointer>

namespace bitcoin {
class CTxDataBase;
class CWallet;
class ECCVerifyHandle;
}

class AssetsTransactionsCache;

class EmulatorWalletDataSource : public WalletDataSource,
                                 public BlockFilterMatchable,
                                 public AbstractKeychain {
    Q_OBJECT
public:
    explicit EmulatorWalletDataSource(const WalletAssetsModel& assetsModel,
        const AssetsTransactionsCache& transactionsCache, QObject* parent = nullptr);

    ~EmulatorWalletDataSource() override;

    Promise<void> createWalletWithMnemonic() override;
    Promise<void> loadWallet(std::optional<SecureString> password) override;
    Promise<void> encryptWallet(SecureString password);
    Promise<MutableTransactionRef> createSendTransaction(bitcoin::UTXOSendTxParams params) override;
    Promise<void> restoreWalletWithMnemAndPass(QString mnemonic) override;
    Promise<QString> getReceivingAddress(AssetID id, Enums::AddressType addressType) const override;
    Promise<QString> getChangeAddress(AssetID id, Enums::AddressType addressType) const override;
    Promise<QString> getMnemonic() const override;
    Promise<AddressesList> getAllKnownAddressesById(AssetID assetId) const override;
    Promise<void> setAddressType(AssetID assetID, Enums::AddressType addressType) override;
    Promise<Enums::AddressType> getAddressType(AssetID assetID) const override;
    Enums::AddressType getAddressTypeSync(AssetID assetID) const override;
    Promise<bool> isOurAddress(
        AssetID assetID, std::vector<unsigned char> scriptPubKey) const override;

    bool isCreated() const override;
    bool isLoaded() const override;
    bool isEncrypted() const override;
    bool isEmpty(AssetID assetID) const override;

    QString identityPubKey() const override;

    OnChainTxRef applyTransaction(OnChainTxRef source, LookupTxById lookupTx) override;
    Promise<OnChainTxRef> applyTransactionAsync(OnChainTxRef source) override;
    void undoTransaction(OnChainTxRef transaction) override;
    BFMatcherUniqueRef createMatcher(AssetID assetID) const override;

    Promise<QByteArray> dumpPrivKey(
        AssetID assetID, std::vector<unsigned char> scriptPubKey) const override;

    // AbstractKeychain interface
public:
    Promise<KeyDescriptor> deriveNextKey(AssetID assetID, uint32_t keyFamily) override;
    Promise<KeyDescriptor> deriveKey(AssetID assetID, KeyLocator locator) override;
    Promise<std::string> derivePrivKey(AssetID assetID, KeyDescriptor descriptor) override;

private:
    void init();
    void markAddressAsUsed(AssetID id, QString address);

private:
    QObject* _executionContext{ nullptr };
    const WalletAssetsModel& _walletAssetsModel;
    const AssetsTransactionsCache& _transactionsCache;
    std::unique_ptr<bitcoin::ECCVerifyHandle> _secp256k1VerifyHandle;

    bool _walletCreated{ true };
    bool _walletLoaded{ false };
    std::unique_ptr<bitcoin::CWallet> _wallet;
    TransactionsList _emptyList;
    std::map<AssetID, Enums::AddressType> _addressType;
};

#endif // EMULATORWALLETDATASOURCE_HPP
