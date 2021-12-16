#include "Wallet.hpp"

#include <Chain/AbstractChainManager.hpp>
#include <Chain/AbstractTransactionsCache.hpp>
#include <Chain/BlockIndex.hpp>
#include <Chain/Chain.hpp>
#include <Data/WalletAssetsModel.hpp>
#include <EthCore/Types.hpp>
#include <Tools/Common.hpp>
#include <Utils/Logging.hpp>
#include <bip39.h>
#include <golomb/gcs.h>
#include <hdchain.h>
#include <interfaces.hpp>
#include <iostream>
#include <key_io.h>
#include <outputtype.h>
#include <random.h>
#include <script/script.h>
#include <string>
#include <tinyformat.h>
#include <utilstrencodings.h>
#include <wallet.h>
#include <walletdb.h>

#include <QDir>
#include <QFileInfo>
#include <QMetaObject>
#include <QStandardPaths>
#include <QVector>
#include <boost/multiprecision/cpp_dec_float.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>
#include <chrono>
#include <string>
#include <tinyformat.h>

using namespace boost::adaptors;

//==============================================================================

struct AutoResetCWallet {
    std::unique_ptr<bitcoin::CWallet> wallet;
    std::function<void(void)> cleanup;

    AutoResetCWallet(std::unique_ptr<bitcoin::CWallet>&& w, std::function<void(void)> cleanup)
        : wallet(std::move(w))
        , cleanup(cleanup)
    {
    }

    AutoResetCWallet(AutoResetCWallet&& rhs) noexcept
        : wallet(std::move(rhs.wallet))
        , cleanup(std::move(rhs.cleanup))
    {
    }

    ~AutoResetCWallet()
    {
        if (wallet) {
            cleanup();
        }
    }
};

//==============================================================================

static AutoResetCWallet CreateWalletDb(bool isEmulated)
{
    try {
        return AutoResetCWallet(std::make_unique<bitcoin::CWallet>(GetPathForWalletDb(isEmulated)),
            [isEmulated] { ResetWallet(isEmulated); });

    } catch (std::exception& ex) {
        LogCDebug(WalletBackend) << ex.what();
        throw;
    }
}

//==============================================================================

static bitcoin::CTxDestination AddressFromKeyID(bitcoin::CKeyID keyID, Enums::AddressType type)
{
    switch (type) {
    case Enums::AddressType::P2PKH:
        return bitcoin::GetDestinationForKey(keyID, bitcoin::OutputType::LEGACY);
    case Enums::AddressType::P2WPKH:
        return bitcoin::GetDestinationForKey(keyID, bitcoin::OutputType::BECH32);
    case Enums::AddressType::P2WSH:
        return bitcoin::GetDestinationForKey(keyID, bitcoin::OutputType::P2SH_SEGWIT);
    }

    return bitcoin::CTxDestination();
}

//==============================================================================

static std::vector<bitcoin::CKeyID> GetKeyIDPool(
    bitcoin::CWallet* wallet, AssetID id, bool isChange)
{
    return wallet->GetKeyPoolKeys(id, 0, isChange);
}

//==============================================================================

static std::vector<bitcoin::CScript> GetAddressPool(
    bitcoin::CWallet* wallet, AssetID assetId, AddressTypes types, bool isChange)
{
    std::vector<bitcoin::CScript> result;
    auto keyIDPool = GetKeyIDPool(wallet, assetId, isChange);
    for (auto&& type : types) {
        std::transform(std::begin(keyIDPool), std::end(keyIDPool), std::back_inserter(result),
            [type](const bitcoin::CKeyID& keyID) {
                return bitcoin::GetScriptForDestination(AddressFromKeyID(keyID, type));
            });
    }

    return result;
}

//==============================================================================

static std::optional<bitcoin::CPubKey> GetAccountPubKey(
    bitcoin::CWallet& wallet, const CoinAsset& asset)
{
    bitcoin::CPubKey pubkey;
    auto extCoinType = asset.params().params.extCoinType();
    wallet.TopUpKeyPoolByAsset(44, extCoinType, { 0 }, 0, 1);
    if (wallet.FindHDPubKey(extCoinType, 0, false, 0, pubkey)) {
        return std::make_optional(pubkey);
    } else {
        return std::nullopt;
    }
}

//==============================================================================

static MutableTransactionRef BitcoinTxToMutableTx(
    AssetID assetID, const bitcoin::CTransaction& bitcoinTx)
{
    auto mutableTx = std::make_shared<MutableTransaction>(assetID);
    std::vector<MutableTransaction::TxIn> inputs;

    std::transform(std::begin(bitcoinTx.vin), std::end(bitcoinTx.vin), std::back_inserter(inputs),
        [](const auto& in) {
            std::vector<unsigned char> scriptSig(in.scriptSig.begin(), in.scriptSig.end());
            return std::make_tuple(in.prevout.hash.ToString(), in.prevout.n, scriptSig);
        });

    std::vector<MutableTransaction::TxOut> outputs;
    std::transform(std::begin(bitcoinTx.vout), std::end(bitcoinTx.vout),
        std::back_inserter(outputs), [](const auto& out) -> MutableTransaction::TxOut {
            return std::make_tuple(ToByteVector(out.scriptPubKey), out.nValue);
        });

    mutableTx->addInputs(inputs);
    mutableTx->addOutputs(outputs);
    mutableTx->setHashAndHex(bitcoinTx.GetHash().ToString(), EncodeHexTx(bitcoinTx));
    return mutableTx;
}

//==============================================================================

static QString DBErrorToStringError(bitcoin::DBErrors error)
{
    switch (error) {
    case bitcoin::DBErrors::DB_LOAD_FAIL:
        return QString("Load fail");
    case bitcoin::DBErrors::DB_LOAD_OK:
        return QString("Load ok");
    case bitcoin::DBErrors::DB_TOO_NEW:
        return QString("Too new");
    case bitcoin::DBErrors::DB_CORRUPT:
        return QString("Corrupt");
    case bitcoin::DBErrors::DB_NEED_REWRITE:
        return QString("Need rewrite");
    case bitcoin::DBErrors::DB_NONCRITICAL_ERROR:
        return QString("Nonclitical error");
    }
}

//==============================================================================

static void LoadWallet(bitcoin::CWallet* wallet)
{
    bool firstRun = true;
    bitcoin::DBErrors error = wallet->LoadWallet(firstRun);
    if (error != bitcoin::DBErrors::DB_LOAD_OK) {
        throw std::runtime_error(DBErrorToStringError(error).toStdString());
    }
}

//==============================================================================

static bitcoin::CChainParams GetChainParams(const WalletAssetsModel& assetsModel, AssetID id)
{
    return assetsModel.assetById(id).params().params;
}

//==============================================================================

static bitcoin::CKeyID GetKeyID(bitcoin::CWallet* wallet, AssetID id, bool isChange)
{
    return wallet->GetLastUnusedKey(id, 0, isChange).GetID();
}

//==============================================================================

static AddressesList BitcoinAddrListToAddrList(bitcoin::CWallet::AddressesList list)
{
    AddressesList result;
    boost::copy(list | transformed(QString::fromStdString), std::back_inserter(result));
    return result;
}

//==============================================================================

static QString EncodeAddress(
    const WalletAssetsModel& assetsModel, AssetID id, const bitcoin::CTxDestination& dest)
{
    return QString::fromStdString(
        bitcoin::EncodeDestination(dest, GetChainParams(assetsModel, id)));
}

//==============================================================================

Wallet::Wallet(const WalletAssetsModel& assetsModel, AssetsTransactionsCache& transactionsCache,
    const AbstractMutableChainManager& chainManager, bool emulated, QObject* parent)
    : WalletDataSource(parent)
    , _executionContext(new QObject(this))
    , _assetsModel(assetsModel)
    , _transactionsCache(transactionsCache)
    , _chainManager(chainManager)
    , _emulated(emulated)
{
    init();
}

//==============================================================================

Wallet::~Wallet()
{
    bitcoin::ECC_Stop();
}

//==============================================================================

void Wallet::maintainUTXOSet(const OnChainTx& transaction)
{
    if (transaction.isConflicted()) {
        return; // won't apply conflicted transaction
    }

    bitcoin::uint256 txHash = bitcoin::uint256S(transaction.txId().toStdString());
    auto assetID = transaction.assetID();
    //    ConflictingInputs conflictingInputs;
    std::vector<bitcoin::COutPoint> validSpendOutpoints;
    for (auto&& in : transaction.inputs()) {
        bitcoin::uint256 prevoutHash = bitcoin::uint256S(in.hash());
        bitcoin::COutPoint outpoint(prevoutHash, static_cast<unsigned>(in.index()));

        // NOTE: This doesn't seem to work, so commenting out and getting back to conflicted
        // transactions latter.
        //        bool isUnspent = _utxoSet.hasCoin(assetID, outpoint);
        //        if(isUnspent)
        //        {
        //            validSpendOutpoints.push_back(outpoint);
        //        }
        //        else if(_utxoSet.isSpent(assetID, outpoint))
        //        {
        //            conflictingInputs[in] = _utxoSet.outpointSpentIn(assetID, outpoint);
        //        }

        // TEMP solution since conflicts doesn't seem to work.
        _utxoSet.spendUTXO(assetID, outpoint, transaction.txId());
    }

    //    if(!conflictingInputs.empty())
    //    {
    //        transaction.setConflictedInputs(conflictingInputs);
    //        return;
    //    }
    //    else
    //    {
    //        boost::for_each(validSpendOutpoints, [this, assetID, &transaction](const auto
    //        &outpoint) {
    //            _utxoSet.spendUTXO(assetID, outpoint, transaction.txId());
    //        });
    //    }

    auto params = GetChainParams(_assetsModel, assetID);
    for (auto&& out : transaction.outputs()) {
        auto dest = bitcoin::DecodeDestination(out.address(), params);
        bitcoin::CTxOut txOut(out.value(), bitcoin::GetScriptForDestination(dest));
        _utxoSet.addUnspentUTXO(assetID, bitcoin::COutPoint(txHash, out.index()), txOut);
        markAddressAsUsed(assetID, QString::fromStdString(out.address()));
    }

    _transactionsApplied[assetID].insert(transaction.txId());
}

//==============================================================================

void Wallet::applyTransactionHelper(const OnChainTx& filteredTx)
{
    if (_transactionsApplied.count(filteredTx.assetID()) > 0
        && _transactionsApplied.at(filteredTx.assetID()).count(filteredTx.txId()) > 0) {
        // already applied no need to affect UTXO set.
        return;
    }

    auto assetID = filteredTx.assetID();
    for (auto&& out : filteredTx.outputs()) {
        markAddressAsUsed(assetID, QString::fromStdString(out.address()));
    }

    maintainUTXOSet(filteredTx);
}

//==============================================================================

Promise<QString> Wallet::getReceivingAddress(AssetID id, Enums::AddressType addressType) const
{
    return Promise<QString>([this, addressType, id](const auto& resolver, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            auto keyID = GetKeyID(_wallet.get(), id, false);

            auto result = EncodeAddress(_assetsModel, id, AddressFromKeyID(keyID, addressType));

            if (result.isEmpty()) {
                reject(result);
            } else {
                resolver(result);
            }
        });
    });
}

//==============================================================================

Promise<QString> Wallet::getChangeAddress(AssetID id, Enums::AddressType addressType) const
{
    return Promise<QString>([this, addressType, id](const auto& resolver, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            auto keyID = GetKeyID(_wallet.get(), id, true);
            auto result = EncodeAddress(_assetsModel, id, AddressFromKeyID(keyID, addressType));
            if (result.isEmpty()) {
                reject(result);
            } else {
                resolver(result);
            }
        });
    });
}

//==============================================================================

void Wallet::markAddressAsUsed(AssetID id, QString address)
{
    auto chainParams = GetChainParams(_assetsModel, id);
    _wallet->MarkAddressAsUsed(
        bitcoin::DecodeDestination(address.toStdString(), chainParams), chainParams, id);
}

//==============================================================================

OnChainTxRef Wallet::tryApplyTransactionHelper(
    OnChainTxRef source, LookupTxById extraLookupTx) const
{
    OnChainTx::Inputs inputs;
    OnChainTx::Outputs outputs;

    auto assetId = source->assetID();
    OnChainTxRef result;
    const auto& params = GetChainParams(_assetsModel, assetId);

    const auto& cache = _transactionsCache.cacheByIdSync(assetId);

    int64_t totalInputs = 0;
    int64_t delta = 0;
    bool hasScriptInput = false;

    LookupTxById baseLookupTx = [&cache](QString txId) { return cache.transactionByIdSync(txId); };

    LookupTxById lookupTx = extraLookupTx ? [&extraLookupTx, &baseLookupTx](QString txId) {
        if (auto ret = extraLookupTx(txId)) {
            return ret;
        }

        return baseLookupTx(txId);
    } : baseLookupTx;

    for (const auto& outpoint : source->inputs()) {
        if (auto prevTx = lookupTx(QString::fromStdString(outpoint.hash()))) {
            if (auto output = TransactionUtils::FindOutput(*prevTx, outpoint.index())) {
                auto dest = DecodeDestination(output->address(), params);
                hasScriptInput |= (boost::get<bitcoin::WitnessV0ScriptHash>(&dest) != nullptr);
                auto keyid = boost::apply_visitor(bitcoin::CTxDestinationToKeyIDVisitor(), dest);
                totalInputs += output->value();
                if (!keyid.IsNull() && _wallet->HaveKey(assetId, keyid)) {
                    inputs.emplace_back(outpoint);
                    delta -= output->value();
                }
            } else if (!hasScriptInput) {
                // didn't find output, but it can be that it's close channel tx, so let's verify
                // that prev tx is open channel tx
                hasScriptInput = prevTx->type()
                    == chain::OnChainTransaction::TxType::OnChainTransaction_TxType_OPEN_CHANNEL;
            }
        }
    }

    int64_t totalOutputs = 0;

    bool hasScriptOutput = false;

    for (const auto& output : source->outputs()) {
        auto dest = bitcoin::DecodeDestination(output.address(), params);
        auto keyid = boost::apply_visitor(bitcoin::CTxDestinationToKeyIDVisitor(), dest);
        hasScriptOutput |= (boost::get<bitcoin::WitnessV0ScriptHash>(&dest) != nullptr);
        totalOutputs += output.value();
        if (!keyid.IsNull() && _wallet->HaveKey(assetId, keyid)) {
            outputs.emplace_back(output);
            delta += output.value();
        }
    }

    if (outputs.size() + inputs.size() > 0) {
        result = std::make_shared<OnChainTx>(source->assetID(), source->txId(), source->blockHash(),
            source->blockHeight(), source->transactionIndex(), source->transactionDate(), inputs,
            outputs, source->type(), TxMemo{});

        if (hasScriptOutput && delta < 0) {
            result->tx().set_type(
                chain::OnChainTransaction::TxType::OnChainTransaction_TxType_OPEN_CHANNEL);
        }

        if (hasScriptInput && delta > 0) {
            result->tx().set_type(
                chain::OnChainTransaction::TxType::OnChainTransaction_TxType_CLOSE_CHANNEL);
        }

        if (totalInputs > totalOutputs) {
            result->tx().set_fee(totalInputs - totalOutputs);
        }
        result->setHexSerialization(source->serialized().toStdString());
    }

    return result;
}

//==============================================================================

void Wallet::updateIsWalletEncrypted()
{
    _isEncrypted = bitcoin::CWallet(GetPathForWalletDb(_emulated)).IsEncrypted();
    isWalletCryptedChanged();
}

//==============================================================================

Promise<QString> Wallet::getMnemonic() const
{
    return Promise<QString>([this](const auto& resolver, const auto&) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            bitcoin::CHDChain hdChain;
            _wallet->GetDecryptedHDChain(hdChain);
            SecureString mnemonicResult, paraphraseResult;
            hdChain.GetMnemonic(mnemonicResult, paraphraseResult);
            resolver(QString::fromStdString(
                std::string{ mnemonicResult.begin(), mnemonicResult.end() }));
        });
    });
}

//==============================================================================

bool Wallet::isCreated() const
{
    auto folderPath = QString::fromStdString(GetPathForWalletDb(_emulated));
    if (QFileInfo(folderPath).exists()) {
        auto entries
            = QDir(folderPath)
                  .entryList(QDir::NoSymLinks | QDir::NoDotAndDotDot | QDir::Files, QDir::NoSort);
        return entries.size() > 0;
    }

    return false;
}

//==============================================================================

bool Wallet::isEncrypted() const
{
    return _isEncrypted;
}

//==============================================================================

bool Wallet::isLoaded() const
{
    return _isLoaded;
}

//==============================================================================

bool Wallet::isEmpty(AssetID assetID) const
{
    return std::find(std::begin(_notEmptyAssets), std::end(_notEmptyAssets), assetID)
        == std::end(_notEmptyAssets);
}

//==============================================================================

QString Wallet::identityPubKey() const
{
    Q_ASSERT_X(isLoaded(), __FUNCTION__, "Wallet must be loaded");
    Q_ASSERT_X(!_wallet->identityPubKey.empty(), __FUNCTION__, "Identity pubkey must be not null");

    return QString::fromStdString(_wallet->identityPubKey);
}

//==============================================================================

OnChainTxRef Wallet::applyTransaction(OnChainTxRef source, LookupTxById lookupTx)
{
    if (auto result = tryApplyTransactionHelper(source, lookupTx)) {

        auto inputsCompare = [](const auto& lhs, const auto& rhs) {
            return lhs.index() == rhs.index() && lhs.hash() == rhs.hash();
        };
        auto outputsCompare = [](const auto& lhs, const auto& rhs) {
            return lhs.value() == rhs.value() && lhs.address() == rhs.address()
                && lhs.index() == rhs.index();
        };

        if (boost::equal(result->inputs(), source->inputs(), inputsCompare)
            && boost::equal(result->outputs(), source->outputs(), outputsCompare)) {
            result->tx().mutable_memo()->insert({ OnChainTx::MEMO_TX_TYPE_FLAG,
                std::to_string(Enums::TransactionFlags::PaymentToMyself) });
        }

        applyTransactionHelper(*result);
        return result;
    }

    return {};
}

//==============================================================================

Promise<OnChainTxRef> Wallet::applyTransactionAsync(OnChainTxRef source)
{
    return Promise<OnChainTxRef>([this, source](const auto& resolver, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            auto rx = this->applyTransaction(source, {});
            rx ? resolver(rx) : reject(nullptr);
        });
    });
}

//==============================================================================

void Wallet::undoTransaction(OnChainTxRef transaction)
{
    auto assetID = transaction->assetID();
    auto txid = bitcoin::uint256S(transaction->txId().toStdString());
    if (auto appliedTx = tryApplyTransactionHelper(transaction)) {
        for (auto&& input : appliedTx->inputs()) {
            bitcoin::COutPoint prevInput(bitcoin::uint256S(input.hash()), input.index());
            if (auto tx = _transactionsCache.cacheByIdSync(assetID).transactionByIdSync(
                    QString::fromStdString(input.hash()))) {
                if (auto output = TransactionUtils::FindOutput(*tx, input.index())) {
                    auto chainparams = GetChainParams(this->_assetsModel, assetID);
                    auto script = GetScriptForDestination(
                        DecodeDestination((*output).address(), chainparams));
                    bitcoin::CTxOut btcOut(output->value(), script);
                    this->_utxoSet.addUnspentUTXO(assetID, prevInput, btcOut);
                }
            }
        }

        for (auto&& output : appliedTx->outputs()) {
            this->_utxoSet.removeUnspentUTXO(assetID, bitcoin::COutPoint(txid, output.index()));
        }
    }
}

//==============================================================================

Promise<AddressesList> Wallet::getAllKnownAddressesById(AssetID assetID) const
{
    return Promise<AddressesList>([this, assetID](const auto& resolver, const auto&) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            resolver(BitcoinAddrListToAddrList(_wallet->GetAddressesByAssetID(assetID, false)));
        });
    });
}

//==============================================================================

Promise<void> Wallet::setAddressType(AssetID assetID, Enums::AddressType addressType)
{
    return Promise<void>([this, assetID, addressType](const auto& resolver, const auto&) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            _wallet->SetAddressType(assetID, AddressTypeToString(addressType));
            resolver();
        });
    });
}

//==============================================================================

Promise<Enums::AddressType> Wallet::getAddressType(AssetID assetID) const
{
    return Promise<Enums::AddressType>([this, assetID](const auto& resolver, const auto&) {
        QMetaObject::invokeMethod(
            _executionContext, [=] { resolver(this->getAddressTypeSync(assetID)); });
    });
}

//==============================================================================

Enums::AddressType Wallet::getAddressTypeSync(AssetID assetID) const
{
    return AddressTypeStrToEnum(_wallet->GetAddressType(assetID));
}

//==============================================================================

BFMatcherUniqueRef Wallet::createMatcher(AssetID assetID) const
{
    struct Matcher : public BlockFilterMatcher {
        explicit Matcher(AssetID assetID, const Wallet& wallet)
            : BlockFilterMatcher(assetID)
        {
            auto addressesFromPoolHelper = [&wallet, assetID](bool isChange) {
                auto addresses = GetAddressPool(wallet._wallet.get(), assetID,
                    { wallet.getAddressTypeSync(assetID) }, isChange);
                std::vector<std::string> result(addresses.size());
                std::transform(std::begin(addresses), std::end(addresses), std::begin(result),
                    [](const auto& script) { return std::string(script.begin(), script.end()); });
                return result;
            };

            auto usedAddresses = [&wallet, assetID](bool isChange) {
                auto addresses = wallet._wallet->GetAddressesByAssetID(assetID, isChange);
                std::vector<std::string> result(addresses.size());
                std::transform(std::begin(addresses), std::end(addresses), std::begin(result),
                    [&wallet, assetID](const auto& strAddress) {
                        auto dest = bitcoin::DecodeDestination(
                            strAddress, GetChainParams(wallet._assetsModel, assetID));
                        auto script = bitcoin::GetScriptForDestination(dest);
                        return std::string(script.begin(), script.end());
                    });
                return result;
            };

            addressList = Utils::Combine(Utils::Combine(usedAddresses(false), usedAddresses(true)),
                Utils::Combine(addressesFromPoolHelper(false), addressesFromPoolHelper(true)));
        }
        bool match(const BlockIndex& blockIndex) const override
        {
            const auto& encodedFilter = blockIndex.filter();
            if (encodedFilter.isValid()) {
                bitcoin::Filter::key_t key;
                auto hashBlock = bitcoin::uint256S(blockIndex.hash().toStdString());
                std::memcpy(&key[0], hashBlock.begin(), key.size());

                bitcoin::Filter filter = bitcoin::Filter::FromNMPBytes(
                    key, encodedFilter.n, encodedFilter.m, encodedFilter.p, encodedFilter.bytes);

                return filter.matchAny(addressList);
            }
            return false;
        }

        std::vector<std::string> addressList;
        std::vector<std::string> scriptsList;
    };

    return BFMatcherUniqueRef(new Matcher(assetID, *this));
}

//==============================================================================

Promise<void> Wallet::loadWallet(std::optional<SecureString> password)
{
    return Promise<void>([this, password](const auto& resolver, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            try {
                if (_isLoaded) {
                    resolver();
                    return;
                }

                auto wallet = std::make_unique<bitcoin::CWallet>(GetPathForWalletDb(_emulated));

                LoadWallet(wallet.get());

                if (wallet->IsCrypted() && !wallet->Unlock(password.value_or(SecureString{}))) {
                    throw std::runtime_error("The password you entered is incorrect");
                }

                this->initHdAccounts(*wallet);
                this->initAddressTypes(*wallet);

                _wallet = std::move(wallet);
                for (auto&& it : _wallet->mapAddressBook) {
                    _notEmptyAssets.push_back(it.first);
                }

                _isLoaded = true;
                resolver();
            } catch (std::exception&) {
                reject(std::current_exception());
            }
        });
    });
}

//==============================================================================

Promise<void> Wallet::encryptWallet(SecureString password)
{
    return Promise<void>([this, password](const auto& resolver, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            try {
                if (_wallet->EncryptWallet(password)) {
                    setIsWalletEncrypted(true);
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

Promise<void> Wallet::createWalletWithMnemonic()
{
    return Promise<void>([this](const auto& resolver, const auto&) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            auto autoResetWallet = CreateWalletDb(_emulated);
            auto& wallet = *autoResetWallet.wallet;

            auto mnemonic = bitcoin::CMnemonic::Generate(256);
            LoadWallet(&wallet);
            wallet.GenerateNewHDChain(
                std::string(), std::string{ mnemonic.begin(), mnemonic.end() }, std::string());
            this->initHdAccounts(wallet);
            this->initAddressTypes(wallet);
            _wallet = std::move(autoResetWallet.wallet);
            _isLoaded = true;
            resolver();
        });
    });
}

//==============================================================================

Promise<void> Wallet::restoreWalletWithMnemAndPass(QString mnemonic)
{
    return Promise<void>([this, mnemonic](const auto& resolver, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            try {
                auto autoResetWallet = CreateWalletDb(_emulated);
                auto& wallet = *autoResetWallet.wallet;
                LoadWallet(&wallet);
                wallet.GenerateNewHDChain(std::string(), mnemonic.toStdString(), std::string());
                this->recoverAuxChain(wallet);
                this->initHdAccounts(wallet);
                this->initAddressTypes(wallet);
                _wallet = std::move(autoResetWallet.wallet);
                _isLoaded = true;
                resolver();
            } catch (...) {
                reject(std::current_exception());
                return;
            }
        });
    });
}

//==============================================================================

Promise<MutableTransactionRef> Wallet::createSendTransaction(bitcoin::UTXOSendTxParams params)
{
    Q_ASSERT(_chainManager.hasChain(params.assetID));
    auto bestHeight = _chainManager.chainById(params.assetID).getHeight();
    return Promise<MutableTransactionRef>([=](const auto& resolver, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            LogCDebug(WalletBackend)
                << "Creating send tx:" << params.assetID << params.amount << params.addressTo;
            bitcoin::CRecipient recipient;
            recipient.fSubtractFeeFromAmount = params.substractFeeFromAmount;
            recipient.nAmount = qRound64(params.amount * bitcoin::COIN);
            auto chainparams = GetChainParams(_assetsModel, params.assetID);
            recipient.scriptPubKey = bitcoin::GetScriptForDestination(
                bitcoin::DecodeDestination(params.addressTo.toStdString(), chainparams));
            LogCDebug(WalletBackend)
                << "Recv script:" << bitcoin::ScriptToAsmStr(recipient.scriptPubKey).c_str();
            if (recipient.scriptPubKey.empty()) {
                std::string error = "Invalid recipient address";
                LogCDebug(WalletBackend) << "Error:" << error.c_str();
                reject(error);
                return;
            }

            bitcoin::CTransactionRef bitcoinTx;
            bitcoin::CAmount feeRet;
            boost::optional<bitcoin::CFeeRate> feeRate;
            if (params.feeRate.has_value()) {
                feeRate.emplace(*params.feeRate);
            }
            int changePos = -1;
            std::string errorString;
            try {
                Interfaces interfaces = this->createInterfaces(params.assetID, bestHeight);
                auto locked_chain = interfaces.chain->lock();
                _wallet->CreateTransaction(*locked_chain, *interfaces.coinsView,
                    *interfaces.keySource, params.assetID, { recipient }, bitcoinTx, feeRate,
                    feeRet, changePos, errorString);
            } catch (std::exception& ex) {
                LogCDebug(WalletBackend) << QString::fromStdString(ex.what());
                reject(ex.what());
                return;
            }

            if (bitcoinTx) {
                LogCDebug(WalletBackend) << "bitcoin tx:" << bitcoinTx->ToString().c_str();
                auto mutableTx = BitcoinTxToMutableTx(params.assetID, *bitcoinTx);
                LogCDebug(WalletBackend) << "Created tx:"
                                         << "hash:" << mutableTx->txId().c_str() << Qt::endl
                                         << "encoded:" << mutableTx->serialized().c_str();
                mutableTx->setChangePos(changePos);
                mutableTx->setFee(feeRet);
                mutableTx->setRecipientAddress(params.addressTo);
                resolver(mutableTx);
            } else {
                LogCDebug(WalletBackend) << "Error:" << errorString.c_str();
                reject(errorString);
            }
        });
    });
}

//==============================================================================

Promise<eth::SignedTransaction> Wallet::createSendTransaction(eth::AccountSendTxParams params)
{
    return Promise<eth::SignedTransaction>([=](const auto& resolver, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            try {
                const auto& asset = _assetsModel.assetById(params.assetID);
                auto chainId = asset.params().chainId;

                if (!chainId.has_value()) {
                    throw std::runtime_error(
                        "Trying to create a transaction without knowing chainId");
                }
                if (params.addressTo.size() != 42) {
                    throw std::runtime_error("Invalid eth address size!");
                }

                bitcoin::CPubKey pubkey;
                bitcoin::CKey secret;
                if (auto pubkey = GetAccountPubKey(*_wallet, asset)) {
                    if (!_wallet->GetKey(
                            asset.params().params.extCoinType(), pubkey->GetID(), secret)) {
                        throw std::runtime_error("No secret for eth account");
                    }
                } else {
                    throw std::runtime_error("Couldn't find pubkey for eth account");
                }

                auto token = asset.token();
                auto denomination = token ? eth::exp10(token->decimals()) : eth::ether;

                boost::multiprecision::cpp_dec_float_50 wei(params.amount);
                wei *= static_cast<boost::multiprecision::cpp_dec_float_50>(denomination);

                eth::Tx tx;
                tx.nonce = params.nonce;
                tx.gasPrice = params.gasPrice;
                tx.gasLimit = params.gasLimit;

                if (token) {
                    tx.data
                        = eth::erc20::transferPayload(params.addressTo, static_cast<eth::u256>(wei))
                              .toStdString();
                    tx.to = token->contract().toStdString();
                } else {
                    tx.to = params.addressTo.toStdString();
                    tx.data = params.data;
                    tx.value = eth::ConvertToHex(static_cast<eth::u256>(wei));
                }
                resolver(eth::SignTransaction(tx, *chainId, secret));
            } catch (std::exception& ex) {
                reject(std::current_exception());
            } catch (...) {
                LogCDebug(General) << "Critical create transaction error!";
                reject(std::runtime_error("Failed to create transaction!"));
            }
        });
    });
}

//==============================================================================

void Wallet::init()
{
    bitcoin::RandomInit();

    bitcoin::ECC_Start();
    _secp256k1VerifyHandle.reset(new bitcoin::ECCVerifyHandle);

    if (isCreated()) {
        setIsWalletEncrypted(bitcoin::CWallet(GetPathForWalletDb(_emulated)).IsEncrypted());
    }
}

//==============================================================================

void Wallet::initHdAccounts(bitcoin::CWallet& wallet) const
{
    std::vector<AssetID> assets;
    boost::copy(_assetsModel.assets() | transformed(std::mem_fn(&CoinAsset::coinID)),
        std::back_inserter(assets));

    wallet.InitHDChainAccounts(assets);
}

//==============================================================================

void Wallet::initAddressTypes(bitcoin::CWallet& wallet) const
{
    boost::for_each(_assetsModel.assets(), [&wallet](const auto& asset) {
        if (wallet.GetAddressType(asset.coinID()).empty()) {
            std::string addressType;
            if (asset.misc().defaultAddressType != Enums::AddressType::NONE) {
                addressType = AddressTypeToString(asset.misc().defaultAddressType);
            } else {
                auto supportedTypes = GetSupportedAddressTypes(asset);
                if (supportedTypes.count() == 1) {
                    addressType = AddressTypeToString(supportedTypes.first());
                }
            }
            if (!addressType.empty()) {
                wallet.SetAddressType(asset.coinID(), addressType);
            }
        }
    });
}

//==============================================================================

void Wallet::recoverAuxChain(bitcoin::CWallet& wallet) const
{
    std::vector<uint32_t> accounts(10);
    int gen = 0;
    boost::generate(accounts, [&gen] { return gen++; });

    auto pred = [](const auto& asset) { return asset.lndData().supportsLnd; };

    std::vector<AssetID> assets;
    boost::copy(
        _assetsModel.assets() | filtered(pred) | transformed(std::mem_fn(&CoinAsset::coinID)),
        std::back_inserter(assets));

    wallet.RecoverAuxChain(assets, accounts, 300);
}

//==============================================================================

void Wallet::loadUtxoSet()
{
    boost::for_each(_assetsModel.assets(), [this](auto asset) {
        for (auto tx :
            _transactionsCache.cacheByIdSync(asset.coinID()).onChainTransactionsListSync()) {
            this->maintainUTXOSet(*tx);
        }

        LogCDebug(WalletBackend) << "Utxo set, assetID:" << asset.coinID();
        for (auto&& utxo : _utxoSet.allUnspentCoins(asset.coinID())) {
            LogCDebug(WalletBackend) << utxo.ToString().c_str();
        }
    });

    LogCDebug(WalletBackend) << "Finished loading utxo set";
}

//==============================================================================

void Wallet::setIsWalletEncrypted(bool isEncrypted)
{
    if (_isEncrypted != isEncrypted) {
        _isEncrypted = isEncrypted;
        isWalletCryptedChanged();
    }
}

//==============================================================================

Wallet::Interfaces Wallet::createInterfaces(AssetID assetID, size_t bestHeight) const
{
    struct ChainLock : bitcoin::interfaces::Chain::Lock {

        explicit ChainLock(size_t bestHeight)
            : _bestHeight(bestHeight)
        {
        }

        virtual boost::optional<int> getHeight() override { return _bestHeight; }
        virtual boost::optional<int> getBlockHeight(const bitcoin::uint256& hash) override
        {
            return boost::none;
        }
        virtual int getBlockDepth(const bitcoin::uint256& hash) override { return 0; }
        virtual bitcoin::uint256 getBlockHash(int height) override { return bitcoin::uint256(); }
        virtual int64_t getBlockTime(int height) override { return 0; }
        virtual int64_t getBlockMedianTimePast(int height) override { return 0; }
        virtual bool isChainSynced() override { return false; }
        virtual int64_t getBestBlockTime() override { return 0; }
        virtual boost::optional<int> findFirstBlockWithTime(
            int64_t time, bitcoin::uint256* hash) override
        {
            return {};
        }
        virtual boost::optional<int> findFirstBlockWithTimeAndHeight(
            int64_t time, int height) override
        {
            return {};
        }

        size_t _bestHeight;
    };

    struct Chain : bitcoin::interfaces::Chain {

        explicit Chain(size_t bestHeight)
            : _bestHeight(bestHeight)
        {
        }

        //! Return Lock interface. Chain is locked when this is called, and
        //! unlocked when the returned interface is freed.
        virtual std::unique_ptr<Lock> lock(bool try_lock = false) override
        {
            return std::unique_ptr<Lock>(new ChainLock(_bestHeight));
        }

        size_t _bestHeight;
    };

    struct CoinsView : bitcoin::interfaces::CoinsView {

        explicit CoinsView(AssetID assetID, const Wallet& wallet, size_t bestHeight)
            : _assetID(assetID)
            , _wallet(wallet)
            , _bestHeight(bestHeight)
        {
        }

        virtual std::vector<bitcoin::COutput> availableCoins(
            bitcoin::interfaces::Chain::Lock& locked_chain, bool fOnlySafe) const override
        {
            std::vector<bitcoin::COutput> result;
            for (auto&& outpoint : _wallet._utxoSet.allUnspentCoins(_assetID)) {
                if (auto tx
                    = _wallet._transactionsCache.cacheByIdSync(_assetID).transactionByIdSync(
                        QString::fromStdString(outpoint.hash.ToString()))) {
                    if (fOnlySafe) {
                        if (tx->blockHash().isEmpty() || tx->blockHeight() <= 0) {
                            continue;
                        }
                    }

                    if (tx->isConflicted()) {
                        continue;
                    }

                    auto depth = _bestHeight - tx->blockHeight() + 1;
                    result.emplace_back(
                        outpoint, _wallet._utxoSet.accessCoin(_assetID, outpoint), depth, true);
                }
            }

            return result;
        }

        virtual boost::optional<bitcoin::CTxOut> getUTXO(
            bitcoin::interfaces::Chain::Lock& locked_chain,
            bitcoin::COutPoint outpoint) const override
        {
            boost::optional<bitcoin::CTxOut> result;
            if (_wallet._utxoSet.hasCoin(_assetID, outpoint)) {
                result = _wallet._utxoSet.accessCoin(_assetID, outpoint);
            }

            return result;
        }

        AssetID _assetID;
        const Wallet& _wallet;
        size_t _bestHeight;
    };

    struct ChangeKeysSource : bitcoin::interfaces::ReserveKeySource {
        explicit ChangeKeysSource(AssetID assetID, const Wallet& wallet)
            : _assetID(assetID)
            , _wallet(wallet)
        {
        }

        virtual boost::optional<bitcoin::CPubKey> getReservedKey()
        {
            auto changePubKey = _wallet._wallet->GetLastUnusedKey(_assetID, 0, true);
            return boost::make_optional(changePubKey);
        }

        AssetID _assetID;
        const Wallet& _wallet;
    };

    Interfaces result;
    result.chain.reset(new Chain(bestHeight));
    result.coinsView.reset(new CoinsView(assetID, *this, bestHeight));
    result.keySource.reset(new ChangeKeysSource(assetID, *this));

    return result;
}

//==============================================================================

void UTXOSet::addUnspentUTXO(AssetID assetID, bitcoin::COutPoint prevInput, bitcoin::CTxOut output)
{
    _utxoSet[assetID].emplace(prevInput, output);
}

//==============================================================================

void UTXOSet::spendUTXO(AssetID assetID, bitcoin::COutPoint outpoint, QString spendingTransactionId)
{
    if (removeUnspentUTXO(assetID, outpoint)) {
        _spentUTXOSet[assetID][outpoint] = spendingTransactionId;
    }
}

//==============================================================================

bool UTXOSet::removeUnspentUTXO(AssetID assetID, bitcoin::COutPoint outpoint)
{
    if (_utxoSet.count(assetID) > 0) {
        if (_utxoSet.at(assetID).count(outpoint) > 0) {
            _utxoSet.at(assetID).erase(outpoint);
            return true;
        }
    }

    return false;
}

//==============================================================================

void UTXOSet::lockOutpoint(AssetID assetID, bitcoin::COutPoint outpoint)
{
    if (_utxoSet.count(assetID) > 0) {
        _lockedOutpoints[assetID].emplace(outpoint);
    }
}

//==============================================================================

void UTXOSet::unlockOutpoint(AssetID assetID, bitcoin::COutPoint outpoint)
{
    if (_utxoSet.count(assetID) > 0) {
        _lockedOutpoints.at(assetID).erase(outpoint);
    }
}

//==============================================================================

bool UTXOSet::hasCoin(AssetID assetID, const bitcoin::COutPoint& outpoint) const
{
    return (_utxoSet.count(assetID) > 0) && (_utxoSet.at(assetID).count(outpoint) > 0);
}

//==============================================================================

bool UTXOSet::isSpent(AssetID assetID, const bitcoin::COutPoint& outpoint) const
{
    return (_spentUTXOSet.count(assetID) > 0) && (_spentUTXOSet.at(assetID).count(outpoint) > 0);
}

//==============================================================================

bool UTXOSet::isLocked(AssetID assetID, const bitcoin::COutPoint& outpoint) const
{
    return (_lockedOutpoints.count(assetID) > 0)
        && (_lockedOutpoints.at(assetID).count(outpoint) > 0);
}

//==============================================================================

bitcoin::CTxOut UTXOSet::accessCoin(AssetID assetID, const bitcoin::COutPoint& outpoint) const
{
    return _utxoSet.at(assetID).at(outpoint);
}

//==============================================================================

QString UTXOSet::outpointSpentIn(AssetID assetID, const bitcoin::COutPoint& outpoint) const
{
    return _spentUTXOSet.at(assetID).at(outpoint);
}

//==============================================================================

std::vector<bitcoin::COutPoint> UTXOSet::allUnspentCoins(AssetID assetID) const
{
    std::vector<bitcoin::COutPoint> outpoints;
    if (_utxoSet.count(assetID) > 0) {
        const auto& utxoSet = _utxoSet.at(assetID);
        const auto& locked = _lockedOutpoints.count(assetID) > 0 ? _lockedOutpoints.at(assetID)
                                                                 : std::set<bitcoin::COutPoint>{};
        for (auto&& utxo : utxoSet) {
            if (locked.count(utxo.first) == 0) {
                outpoints.push_back(utxo.first);
            }
        }
    }

    return outpoints;
}

//==============================================================================

Promise<boost::optional<Wire::TxOut>> Wallet::getUTXO(
    AssetID assetID, const Wire::OutPoint& outpoint) const
{
    return Promise<boost::optional<Wire::TxOut>>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            bitcoin::COutPoint btcOutpoint(outpoint.hash, outpoint.index);
            boost::optional<Wire::TxOut> result;
            if (_utxoSet.hasCoin(assetID, btcOutpoint)) {
                auto utxo = _utxoSet.accessCoin(assetID, btcOutpoint);
                result = Wire::TxOut{ utxo.nValue, bitcoin::ToByteVector(utxo.scriptPubKey) };
            } else if (!_utxoSet.isSpent(assetID, btcOutpoint)) {
                reject();
                return;
            }

            resolve(result);
        });
    });
}

//==============================================================================

Promise<std::vector<std::tuple<Wire::OutPoint, Wire::TxOut>>> Wallet::listUTXOs(
    AssetID assetID) const
{
    return Promise<std::vector<std::tuple<Wire::OutPoint, Wire::TxOut>>>(
        [=](const auto& resolve, const auto&) {
            QMetaObject::invokeMethod(_executionContext, [=] {
                std::vector<std::tuple<Wire::OutPoint, Wire::TxOut>> result;

                boost::transform(_utxoSet.allUnspentCoins(assetID),
                    boost::back_move_inserter(result), [assetID, this](const auto& outpoint) {
                        auto utxo = _utxoSet.accessCoin(assetID, outpoint);
                        return std::make_tuple(Wire::OutPoint{ outpoint.hash, outpoint.n },
                            Wire::TxOut{ utxo.nValue, ToByteVector(utxo.scriptPubKey) });
                    });

                resolve(result);
            });
        });
}

//==============================================================================

Promise<void> Wallet::lockOutpoint(AssetID assetID, Wire::OutPoint outpoint)
{
    return Promise<void>([=](const auto& resolve, const auto&) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            _utxoSet.lockOutpoint(assetID, { outpoint.hash, outpoint.index });
            resolve();
        });
    });
}

//==============================================================================

Promise<void> Wallet::unlockOutpoint(AssetID assetID, Wire::OutPoint outpoint)
{
    return Promise<void>([=](const auto& resolve, const auto&) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            _utxoSet.unlockOutpoint(assetID, { outpoint.hash, outpoint.index });
            resolve();
        });
    });
}

//==============================================================================

Promise<void> Wallet::load()
{
    return Promise<void>([=](const auto& resolve, const auto&) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            if (!_isUtxoLoaded) {
                this->loadUtxoSet();
                _isUtxoLoaded = true;
            }
            resolve();
        });
    });
}

//==============================================================================

Promise<KeyDescriptor> Wallet::deriveNextKey(AssetID assetID, uint32_t keyFamily)
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

Promise<KeyDescriptor> Wallet::deriveKey(AssetID assetID, KeyLocator locator)
{
    return Promise<KeyDescriptor>(
        [this, locator, assetID](const auto& resolver, const auto& reject) {
            QMetaObject::invokeMethod(_executionContext, [=] {
                try {
                    bitcoin::CExtKey extKey;
                    bitcoin::CHDChain hdChainCurrent;
                    if (!_wallet->GetDecryptedHDChain(hdChainCurrent)) {
                        throw std::runtime_error(
                            std::string(__func__) + ": GetDecryptedHDChain failed");
                    }
                    hdChainCurrent.DeriveChildExtKey(bitcoin::HD_PURPUSE_LND, assetID,
                        locator.first, false, locator.second, extKey);
                    auto pubKey = extKey.key.GetPubKey();
                    if (pubKey.IsValid()) {
                        resolver(KeyDescriptor{ locator, bitcoin::HexStr(pubKey) });
                    } else {
                        LogCCritical(WalletBackend) << "Couldn't derive valid child ext key"
                                                    << locator.first << locator.second;
                        reject(KeyDescriptor{});
                    }
                } catch (std::exception& ex) {
                    reject(ex);
                }
            });
        });
}

//==============================================================================

Promise<std::string> Wallet::derivePrivKey(AssetID assetID, KeyDescriptor descriptor)
{
    return Promise<std::string>([this, descriptor, assetID](
                                    const auto& resolver, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            try {
                using namespace bitcoin;
                auto locator = std::get<0>(descriptor);
                auto pubkey = std::get<1>(descriptor);

                CExtKey extKey;
                auto deriveKey = [this, assetID](int accountIndex, int childIndex) {
                    CExtKey extKey;
                    bitcoin::CHDChain hdChainCurrent;
                    if (!_wallet->GetDecryptedHDChain(hdChainCurrent)) {
                        throw std::runtime_error(
                            std::string(__func__) + ": GetDecryptedHDChain failed");
                    }
                    hdChainCurrent.DeriveChildExtKey(
                        bitcoin::HD_PURPUSE_LND, assetID, accountIndex, false, childIndex, extKey);
                    return extKey;
                };

                CPubKey btcPubkey(ParseHex(pubkey));
                if (pubkey.empty() || locator.second > 0) {

                } else {
                    if (!btcPubkey.IsValid()) {
                        throw std::runtime_error(
                            strprintf("Pubkey %s is not valid", HexStr(btcPubkey)));
                    }

                    bitcoin::CHDPubKey hdPubkey;
                    ;
                    if (_wallet->GetHDPubKey(assetID, btcPubkey.GetID(), hdPubkey)) {
                        extKey = deriveKey(hdPubkey.nAccountIndex, hdPubkey.extPubKey.nChild);
                        if (extKey.key.VerifyPubKey(btcPubkey)) {
                            locator.first = hdPubkey.nAccountIndex;
                            locator.second = hdPubkey.extPubKey.nChild;
                        }
                    }

                    // TODO(yuraolex): refactor this after everyone fixes their DB.
                    // We had broken nAccountIndex resulting in invalid values in wallet db
                    // So currently we need to try both locator and stored value
                    // in future we need to remove double derivation
                }

                extKey = deriveKey(locator.first, locator.second);

                auto privKey = extKey.key;
                if (privKey.IsValid()) {
                    if (btcPubkey.IsValid()) {
                        if (!privKey.VerifyPubKey(btcPubkey)) {
                            LogCCritical(WalletBackend)
                                << "Priv key doesn't match pub key with locator" << locator.first
                                << locator.second;
                            reject(std::runtime_error(
                                strprintf("Priv key doesn't match pub key, locator: (%d, %d)",
                                    locator.first, locator.second)));
                            return;
                        }
                    }

                    resolver(HexStr(privKey));
                } else {
                    LogCCritical(WalletBackend)
                        << "Priv key is not valid" << locator.first << locator.second;
                    reject(std::runtime_error(strprintf("Priv key is not valid, locator: (%d, %d)",
                        locator.first, locator.second)));
                }
            } catch (std::exception& ex) {
                reject(ex);
            }
        });
    });
}

//==============================================================================

Promise<QString> Wallet::dumpPrivKey(AssetID assetID) const
{
    return Promise<QString>([this, assetID](const auto& resolver, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            const auto& asset = _assetsModel.assetById(assetID);
            if (auto pubkey = GetAccountPubKey(*_wallet, asset)) {
                bitcoin::CKey secret;
                if (!_wallet->GetKey(
                        asset.params().params.extCoinType(), pubkey->GetID(), secret)) {
                    reject(std::runtime_error("No secret for eth account"));
                    return;
                }
                resolver(
                    QString::fromStdString("0x" + bitcoin::HexStr(secret.begin(), secret.end())));
            } else {
                reject(std::runtime_error("No pubkey for default account"));
            }
        });
    });
}

//==============================================================================

Promise<QString> Wallet::getAccountAddress(AssetID assetID) const
{
    return Promise<QString>([this, assetID](const auto& resolver, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            if (auto pubkey = GetAccountPubKey(*_wallet, _assetsModel.assetById(assetID))) {
                resolver(QString::fromStdString(eth::EncodeDestination(*pubkey)));
            } else {
                reject(std::runtime_error("No pubkey for default account"));
            }
        });
    });
}

//==============================================================================

Promise<QByteArray> Wallet::dumpPrivKey(
    AssetID assetID, std::vector<unsigned char> scriptPubKey) const
{
    return Promise<QByteArray>(
        [this, assetID, scriptPubKey](const auto& resolver, const auto& reject) {
            QMetaObject::invokeMethod(_executionContext, [=] {
                using namespace bitcoin;
                CKey privKey;
                CTxDestination dest;
                ExtractDestination(CScript(scriptPubKey.begin(), scriptPubKey.end()), dest);
                CKeyID addr = boost::apply_visitor(bitcoin::CTxDestinationToKeyIDVisitor(), dest);
                if (_wallet->GetKey(assetID, addr, privKey)) {
                    CPubKey vchPubKey;
                    if (_wallet->GetPubKey(assetID, addr, vchPubKey) && addr == vchPubKey.GetID()) {
                        if (privKey.VerifyPubKey(vchPubKey)) {
                            resolver(QByteArray::fromStdString(HexStr(privKey)));
                        } else {
                            reject();
                        }
                    } else {
                        reject();
                    }
                } else {
                    reject();
                }
            });
        });
}

//==============================================================================

Promise<bool> Wallet::isOurAddress(AssetID assetID, std::vector<unsigned char> scriptPubKey) const
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
