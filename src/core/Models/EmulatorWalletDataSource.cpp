#include "EmulatorWalletDataSource.hpp"
#include <Chain/AbstractTransactionsCache.hpp>
#include <Chain/BlockIndex.hpp>
#include <Data/WalletAssetsModel.hpp>
#include <Utils/Logging.hpp>
#include <ViewModels/ApplicationViewModel.hpp>
#include <key_io.h>
#include <script/standard.h>
#include <streams.h>
#include <utilstrencodings.h>
#include <wallet.h>
#include <walletdb.h>

#include <QDir>
#include <QMetaObject>
#include <QStandardPaths>
#include <QTimer>
#include <algorithm>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>
#include <map>
#include <set>
#include <vector>

using namespace boost::adaptors;

static const std::string mnemonic(
    "unlock gaze river calm pink tree marriage lyrics purpose bag develop food devote thing "
    "convince learn boat blue romance sail catch odor rain glory");

//==============================================================================

static std::string GetPathForDBEnv()
{
    QDir path(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    const auto& folderName = "db_env";
    if (!path.exists(folderName)) {
        path.mkpath(folderName);
    }

    path.cd(folderName);

    return path.absolutePath().toStdString();
}

//==============================================================================

static std::string GetPathForTxDb()
{
    QDir path(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    const auto& folderName = "db_env";
    if (!path.exists(folderName)) {
        path.mkpath(folderName);
    }

    path.cd(folderName);
    return path.absoluteFilePath("tx.db").toStdString();
}

//==============================================================================

EmulatorWalletDataSource::EmulatorWalletDataSource(const WalletAssetsModel& assetsModel,
    const AssetsTransactionsCache& transactionsCache, QObject* parent)
    : WalletDataSource(parent)
    , _executionContext(new QObject(this))
    , _walletAssetsModel(assetsModel)
    , _transactionsCache(transactionsCache)
{
}

//==============================================================================

EmulatorWalletDataSource::~EmulatorWalletDataSource() {}

//==============================================================================

Promise<void> EmulatorWalletDataSource::loadWallet(std::optional<SecureString> password)
{
    return Promise<void>([this](const auto& resolver, const auto&) {
        if (this->isCreated()) {
            QMetaObject::invokeMethod(_executionContext, [=] {
                _walletLoaded = true;
                this->init();
                resolver();
            });
        } else {
            resolver();
        }
    });
}

//==============================================================================

Promise<void> EmulatorWalletDataSource::encryptWallet(SecureString password)
{
    return Promise<void>([this, password](const auto& resolver, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            try {
                if (_wallet->EncryptWallet(password)) {
                    resolver();
                } else {
                    reject(std::runtime_error("Failed to encrypt wallet"));
                }
            } catch (std::exception&) {
                reject(std::current_exception());
            }
        });
    });
}

//==============================================================================

Promise<void> EmulatorWalletDataSource::createWalletWithMnemonic()
{
    return Promise<void>([this](const auto& resolver, const auto&) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            LogCDebug(General) << "Wallet created";
            this->init();
            _walletCreated = true;
            _walletLoaded = true;
            resolver();
        });
    });
}

//==============================================================================

Promise<void> EmulatorWalletDataSource::restoreWalletWithMnemAndPass(QString mnemonic)
{
    return Promise<void>([this](const auto& resolver, const auto&) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            LogCDebug(General) << "Wallet restored";
            resolver();
        });
    });
}

//==============================================================================

Promise<MutableTransactionRef> EmulatorWalletDataSource::createSendTransaction(bitcoin::UTXOSendTxParams params)
{
    Q_UNUSED(params);
    return Promise<MutableTransactionRef>([this](const auto& resolver, const auto&) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            LogCDebug(General) << "Send transaction created!";
            resolver(nullptr);
        });
    });
}

//==============================================================================

static OnChainTxRef GenerateTransaction(AssetID assetID)
{
    //    Enums::TransactionType type;

    //    if (rand() % 2 == 1) {
    //        type = Enums::TransactionType::Sent;
    //    } else {
    //        type = Enums::TransactionType::Received;
    //    }

    return {};

    //    return std::make_shared<Transaction>(assetID, QString("dsbsgdbfcvafvcvfv%1").arg(assetID),
    //    QString::number(rand() % 1000), QDateTime(QDate(2012, 7, 6), QTime(8, 30, 0)));
}

//==============================================================================

Promise<QString> EmulatorWalletDataSource::getReceivingAddress(
    AssetID id, Enums::AddressType addressType) const
{
    return Promise<QString>([this, id, addressType](const auto& resolver, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            auto keyID = _wallet->GetLastUnusedKey(id, 0, false).GetID();

            resolver(QString::fromStdString(bitcoin::EncodeDestination(
                bitcoin::WitnessV0KeyHash(keyID), _walletAssetsModel.assetById(id).params().params)));
        });
    });
}

//==============================================================================

Promise<QString> EmulatorWalletDataSource::getChangeAddress(
    AssetID id, Enums::AddressType addressType) const
{
    return Promise<QString>([this, id, addressType](const auto& resolver, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            auto keyID = _wallet->GetLastUnusedKey(id, 0, true).GetID();

            resolver(QString::fromStdString(bitcoin::EncodeDestination(
                bitcoin::WitnessV0KeyHash(keyID), _walletAssetsModel.assetById(id).params().params)));
        });
    });
}

//==============================================================================

void EmulatorWalletDataSource::markAddressAsUsed(AssetID id, QString address) {}

//==============================================================================

Promise<QString> EmulatorWalletDataSource::getMnemonic() const
{
    return Promise<QString>(
        [](const auto& resolver, const auto&) { resolver(QString::fromStdString(mnemonic)); });
}

//==============================================================================

OnChainTxRef EmulatorWalletDataSource::applyTransaction(OnChainTxRef source, LookupTxById lookupTx)
{
    std::vector<chain::TxOutpoint> inputs;
    std::vector<chain::TxOutput> outputs;
    OnChainTxRef result;

    if (!outputs.empty()) {

        result = std::make_shared<OnChainTx>(source->assetID(), source->txId(), source->blockHash(),
            source->blockHeight(), source->transactionIndex(),
            source->transactionDate(), inputs, outputs, source->type(),
            TxMemo{});
    }

    return result;
}

//==============================================================================

Promise<OnChainTxRef> EmulatorWalletDataSource::applyTransactionAsync(OnChainTxRef source)
{
    return QtPromise::resolve(source);
}

//==============================================================================

void EmulatorWalletDataSource::undoTransaction(OnChainTxRef transaction) {}

//==============================================================================

Promise<AddressesList> EmulatorWalletDataSource::getAllKnownAddressesById(AssetID assetID) const
{
    return Promise<AddressesList>([this, assetID](const auto& resolver, const auto&) {
        QMetaObject::invokeMethod(_executionContext, [=] { resolver(AddressesList()); });
    });
}

//==============================================================================

Promise<void> EmulatorWalletDataSource::setAddressType(
    AssetID assetID, Enums::AddressType addressType)
{
    return Promise<void>([this, addressType, assetID](const auto& resolver, const auto&) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            _addressType[assetID] = addressType;
            //            _wallet->SetAddressType(assetID, ConvertAddressType(addressType));
            resolver();
        });
    });
}

//==============================================================================

Promise<Enums::AddressType> EmulatorWalletDataSource::getAddressType(AssetID assetID) const
{
    return QtPromise::resolve(getAddressTypeSync(assetID));
}

//==============================================================================

Enums::AddressType EmulatorWalletDataSource::getAddressTypeSync(AssetID assetID) const
{
    return _addressType.count(assetID) > 0 ? _addressType.at(assetID) : Enums::AddressType::NONE;
}

//==============================================================================

Promise<bool> EmulatorWalletDataSource::isOurAddress(
    AssetID assetID, std::vector<unsigned char> scriptPubKey) const
{
    return Promise<bool>([=](const auto& resolver, const auto& /*reject*/) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            using namespace bitcoin;
            CTxDestination dest;
            ExtractDestination(CScript(scriptPubKey.begin(), scriptPubKey.end()), dest);
            resolver(_wallet->HaveKey(
                assetID, boost::apply_visitor(CTxDestinationToKeyIDVisitor(), dest)));
        });
    });
}

//==============================================================================

bool EmulatorWalletDataSource::isCreated() const
{
    return _walletLoaded;
}

//==============================================================================

bool EmulatorWalletDataSource::isLoaded() const
{
    return _walletLoaded;
}

//==============================================================================

bool EmulatorWalletDataSource::isEncrypted() const
{
    return false;
}

//==============================================================================

bool EmulatorWalletDataSource::isEmpty(AssetID assetID) const
{
    return false;
}

//==============================================================================

QString EmulatorWalletDataSource::identityPubKey() const
{
    return QString("emulated-pub-key");
}

//==============================================================================

BFMatcherUniqueRef EmulatorWalletDataSource::createMatcher(AssetID assetID) const
{
    struct Matcher : public BlockFilterMatcher {
        Matcher(AssetID assetID, const EmulatorWalletDataSource& dataSource)
            : BlockFilterMatcher(assetID)
            , _dataSource(dataSource)
        {
        }

        bool match(const BlockIndex& blockIndex) const override
        {
            return true;
            //            return _dataSource._matchList.count(_assetID) > 0 &&
            //            _dataSource._matchList.at(_assetID).contains(blockIndex.header().hash);
        }

        const EmulatorWalletDataSource& _dataSource;
    };

    return BFMatcherUniqueRef(new Matcher(assetID, *this));
}

//==============================================================================

Promise<QByteArray> EmulatorWalletDataSource::dumpPrivKey(
    AssetID assetID, std::vector<unsigned char> scriptPubKey) const
{
    return Promise<QByteArray>([this, assetID, scriptPubKey](const auto& resolver, const auto&) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            using namespace bitcoin;
            CKey privKey;
            CTxDestination dest;
            ExtractDestination(CScript(scriptPubKey.begin(), scriptPubKey.end()), dest);
            CKeyID addr = boost::apply_visitor(bitcoin::CTxDestinationToKeyIDVisitor(), dest);
            _wallet->GetKey(assetID, addr, privKey);
            resolver(QByteArray::fromStdString(HexStr(privKey)));
        });
    });
}

//==============================================================================

void EmulatorWalletDataSource::init()
{

    bitcoin::RandomInit();

    bitcoin::ECC_Start();
    _secp256k1VerifyHandle.reset(new bitcoin::ECCVerifyHandle);

    QDir dbDir(QFileInfo(QString::fromStdString(GetPathForWalletDb(true))).absoluteDir());
    if (dbDir.exists()) {
        dbDir.removeRecursively();
    }

    try {
        _wallet.reset(new bitcoin::CWallet(GetPathForWalletDb(true)));

        bool firstRun = true;
        bitcoin::DBErrors error = _wallet->LoadWallet(firstRun);
        if (error != bitcoin::DBErrors::DB_LOAD_OK) {
            throw std::runtime_error("Failed to load emulated wallet");
        }

        _wallet->GenerateNewHDChain(std::string(), mnemonic, std::string());
        std::vector<AssetID> assets;
        boost::copy(_walletAssetsModel.assets() | transformed(std::mem_fn(&CoinAsset::coinID)),
            std::back_inserter(assets));

        _wallet->InitHDChainAccounts(assets);
    } catch (std::exception& ex) {
        LogCDebug(General) << ex.what();
        throw;
    }
}

//==============================================================================

Promise<KeyDescriptor> EmulatorWalletDataSource::deriveNextKey(AssetID assetID, uint32_t keyFamily)
{
    return Promise<KeyDescriptor>(
        [this, assetID, keyFamily](const auto& resolver, const auto& reject) {
            QMetaObject::invokeMethod(_executionContext, [=] {
                try {
                    auto pubKey = _wallet->GetNewAuxKey(assetID, keyFamily);
                    if (pubKey.IsValid()) {
                        bitcoin::CHDPubKey hdPubKey;
                        _wallet->GetHDPubKey(assetID, pubKey.GetID(), hdPubKey);
                        auto nIndex = hdPubKey.extPubKey.nChild;
                        resolver(KeyDescriptor{
                            KeyLocator{ keyFamily, nIndex }, bitcoin::HexStr(pubKey) });
                    } else {
                        reject(KeyDescriptor{});
                    }
                } catch (std::exception& ex) {
                    reject(ex);
                }
            });
        });
}

//==============================================================================

Promise<KeyDescriptor> EmulatorWalletDataSource::deriveKey(AssetID assetID, KeyLocator locator)
{
    return Promise<KeyDescriptor>(
        [this, locator, assetID](const auto& resolver, const auto& reject) {
            QMetaObject::invokeMethod(_executionContext, [=] {
                try {
                    bitcoin::CExtKey extKey;

                    bitcoin::CHDChain hdChainCurrent;
                    if (!_wallet->GetDecryptedHDChain(hdChainCurrent)) {
                        throw std::runtime_error("Failed to get decrypted hd chain");
                    }

                    hdChainCurrent.DeriveChildExtKey(bitcoin::HD_PURPUSE_LND, assetID,
                        locator.first, false, locator.second, extKey);

                    auto pubKey = extKey.key.GetPubKey();
                    if (pubKey.IsValid()) {
                        resolver(KeyDescriptor{ locator, bitcoin::HexStr(pubKey) });
                    } else {
                        reject(KeyDescriptor{});
                    }
                } catch (std::exception& ex) {
                    reject(ex);
                }
            });
        });
}

//==============================================================================

Promise<std::string> EmulatorWalletDataSource::derivePrivKey(
    AssetID assetID, KeyDescriptor descriptor)
{
    return Promise<std::string>(
        [this, descriptor, assetID](const auto& resolver, const auto& reject) {
            QMetaObject::invokeMethod(_executionContext, [=] {
                try {
                    using namespace bitcoin;
                    CExtKey extKey;
                    auto locator = std::get<0>(descriptor);
                    auto pubkey = std::get<1>(descriptor);

                    if (pubkey.empty() || locator.second > 0) {

                    } else {
                        CPubKey btcPubkey(ParseHex(pubkey));
                        if (!btcPubkey.IsValid()) {
                            throw std::runtime_error("Pubkey is not valid");
                        }

                        bitcoin::CHDPubKey hdPubkey;
                        _wallet->GetHDPubKey(assetID, btcPubkey.GetID(), hdPubkey);
                        locator.first = hdPubkey.nAccountIndex;
                        locator.second = hdPubkey.extPubKey.nChild;
                    }

                    bitcoin::CHDChain hdChainCurrent;
                    if (!_wallet->GetDecryptedHDChain(hdChainCurrent)) {
                        throw std::runtime_error(
                            std::string(__func__) + ": GetDecryptedHDChain failed");
                    }

                    hdChainCurrent.DeriveChildExtKey(
                        HD_PURPUSE_LND, assetID, locator.first, false, locator.second, extKey);
                    auto privKey = extKey.key;
                    if (privKey.IsValid()) {
                        resolver(HexStr(privKey));
                    } else {
                        reject(std::string());
                    }
                } catch (std::exception& ex) {
                    reject(ex);
                }
            });
        });
}

//==============================================================================
