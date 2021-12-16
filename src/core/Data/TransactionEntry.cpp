#include "TransactionEntry.hpp"
#include <Data/WalletAssetsModel.hpp>
#include <EthCore/Encodings.hpp>
#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <key_io.h>
#include <type_traits>
#include <utilstrencodings.h>
#include <walletdb.h>

std::string OnChainTx::MEMO_TX_TYPE_FLAG{ "tx_type" };
const QString EthOnChainTx::UNKNOWN_BLOCK_HASH = QString{};
const int64_t EthOnChainTx::UNKNOWN_BLOCK_HEIGHT = -1;
const int64_t EthOnChainTx::UNKNOWN_GAS_USED = 0;

//==============================================================================

OnChainTxRef TransactionUtils::FindTransaction(const OnChainTxList& where, QString transactionID)
{
    auto it = std::find_if(std::begin(where), std::end(where),
        [transactionID](const auto& entry) { return entry->txId() == transactionID; });

    return it != std::end(where) ? *it : OnChainTxRef();
}

//==============================================================================

boost::optional<chain::TxOutput> TransactionUtils::FindOutput(const OnChainTx& tx, size_t index)
{
    const auto& outputs = tx.tx().outputs();
    auto it = std::find_if(std::begin(outputs), std::end(outputs),
        [index](const auto& output) { return output.index() == index; });

    return it != std::end(outputs) ? boost::make_optional(*it) : boost::none;
}

//==============================================================================

MutableTransaction::MutableTransaction(AssetID assetID)
    : _assetID(assetID)
{
}

//==============================================================================

const std::vector<MutableTransaction::TxIn>& MutableTransaction::inputs() const
{
    return _inputs;
}

//==============================================================================

const std::vector<MutableTransaction::TxOut>& MutableTransaction::outputs() const
{
    return _outputs;
}

//==============================================================================

void MutableTransaction::addInputs(std::vector<MutableTransaction::TxIn> vec)
{
    std::copy(std::begin(vec), std::end(vec), std::back_inserter(_inputs));
    invalidateHash();
}

//==============================================================================

void MutableTransaction::addOutputs(std::vector<MutableTransaction::TxOut> vec)
{
    std::copy(std::begin(vec), std::end(vec), std::back_inserter(_outputs));
    invalidateHash();
}

//==============================================================================

void MutableTransaction::setInput(size_t index, MutableTransaction::TxIn input)
{
    auto oldValue = _inputs.at(index);
    if (oldValue != input) {
        _inputs[index] = input;
        invalidateHash();
    }
}

//==============================================================================

void MutableTransaction::setOutput(size_t index, MutableTransaction::TxOut output)
{
    auto oldValue = _outputs.at(index);
    if (oldValue != output) {
        _outputs[index] = output;
        invalidateHash();
    }
}

//==============================================================================

void MutableTransaction::setHashAndHex(std::string hash, std::string txHex)
{
    _hash = hash;
    _serializedTransactionHex = txHex;
    _hashValid = true;
}

//==============================================================================

bool MutableTransaction::isValid() const
{
    return _hashValid;
}

//==============================================================================

std::string MutableTransaction::txId() const
{
    return _hash;
}

//==============================================================================

std::string MutableTransaction::serialized() const
{
    return _serializedTransactionHex;
}

//==============================================================================

AssetID MutableTransaction::assetID() const
{
    return _assetID;
}

//==============================================================================

int MutableTransaction::changePos() const
{
    return _changePos;
}

//==============================================================================

void MutableTransaction::setChangePos(int changePos)
{
    _changePos = changePos;
}

//==============================================================================

qint64 MutableTransaction::fee() const
{
    return _fee;
}

//==============================================================================

void MutableTransaction::setFee(qint64 fee)
{
    _fee = fee;
}

//==============================================================================

QString MutableTransaction::recipientAddress() const
{
    return _recipientAddress;
}

//==============================================================================

void MutableTransaction::setRecipientAddress(QString recipientAddress)
{
    _recipientAddress = recipientAddress;
}

//==============================================================================

MutableTransaction MutableTransaction::FromRawTx(AssetID assetID, std::string rawTx)
{
    bitcoin::CMutableTransaction tx;
    CDataStream ss(bitcoin::ParseHex(rawTx), bitcoin::SER_NETWORK, bitcoin::PROTOCOL_VERSION);
    ss >> tx;

    MutableTransaction result(assetID);

    for (auto&& txOut : tx.vout) {
        result.addOutputs(
            { std::make_tuple(bitcoin::ToByteVector(txOut.scriptPubKey), txOut.nValue) });
    }

    for (auto&& txIn : tx.vin) {
        result.addInputs({ std::make_tuple(
            txIn.prevout.hash.ToString(), txIn.prevout.n, bitcoin::ToByteVector(txIn.scriptSig)) });
    }

    result.setHashAndHex(tx.GetHash().ToString(), rawTx);

    return result;
}

//==============================================================================

void MutableTransaction::invalidateHash()
{
    _hashValid = false;
}

//==============================================================================

QDebug operator<<(QDebug debug, const Transaction& transactionEntry)
{
    struct Visitor {
        Visitor(QDebug debug)
            : debug(debug)
        {
        }
        void operator()(const OnChainTxRef& tx)
        {
            debug << "Tx: " << tx->txId() << Qt::endl;
            for (auto in : tx->inputs()) {
                debug << '\t' << QString("TxIn: (%1, %2)").arg(in.hash().c_str()).arg(in.index())
                      << Qt::endl;
            }
            for (auto out : tx->outputs()) {
                debug << '\t'
                      << QString("TxOut: n: %1 (%2, %3)")
                             .arg(out.index())
                             .arg(out.address().c_str())
                             .arg(out.value())
                      << Qt::endl;
            }

            debug << Qt::endl;
            debug << "height:" << tx->blockHeight() << Qt::endl;
        }

        void operator()(const LightningInvoiceRef& tx)
        {
            debug << "Lightning invoice" << tx.get() << Qt::endl;
        }

        void operator()(const LightningPaymentRef& tx)
        {
            debug << "Lightning payment" << tx.get() << Qt::endl;
        }

        void operator()(const EthOnChainTxRef& tx) { debug << "Eth tx" << tx.get() << Qt::endl; }

        void operator()(const ConnextPaymentRef& tx)
        {
            debug << "Connext payment tx" << tx.get() << Qt::endl;
        }

        QDebug debug;
    };

    boost::variant2::visit(Visitor(debug), transactionEntry);

    return debug;
}

//==============================================================================

OnChainTxRef TransactionUtils::MutableTxToTransactionRef(
    const MutableTransaction& transaction, const WalletAssetsModel& walletAssetsModel)
{
    const auto& assetID = transaction.assetID();
    const auto& txId = QString::fromStdString(transaction.txId());
    const auto& txDate = QDateTime::currentDateTimeUtc();

    auto mutInputs = transaction.inputs();
    OnChainTx::Inputs inputs;
    std::transform(std::begin(mutInputs), std::end(mutInputs), std::back_inserter(inputs),
        [](const auto& value) {
            chain::TxOutpoint outpoint;
            outpoint.set_hash(std::get<0>(value));
            outpoint.set_index(std::get<1>(value));
            return outpoint;
        });

    auto mutOutputs = transaction.outputs();
    OnChainTx::Outputs outputs;
    for (size_t i = 0; i < mutOutputs.size(); ++i) {
        bitcoin::CTxDestination dest;
        auto scriptRaw = std::get<0>(mutOutputs.at(i));
        bitcoin::CScript script;
        script.assign(scriptRaw.begin(), scriptRaw.end());
        bitcoin::ExtractDestination(script, dest);
        auto address = QString::fromStdString(
            bitcoin::EncodeDestination(dest, walletAssetsModel.assetById(assetID).params().params));

        chain::TxOutput output;
        output.set_index(i);
        output.set_value(std::get<1>(mutOutputs.at(i)));
        output.set_address(address.toStdString());
        outputs.push_back(output);
    }

    auto result = std::make_shared<OnChainTx>(assetID, txId, QString(), -1, 0, txDate, inputs,
        outputs, chain::OnChainTransaction_TxType::OnChainTransaction_TxType_PAYMENT, TxMemo{});
    result->setHexSerialization(transaction.serialized());
    return result;
}

//==============================================================================

BaseTransaction::BaseTransaction(AssetID assetID)
{
    _tx.set_asset_id(assetID);
}

//==============================================================================

BaseTransaction::BaseTransaction(chain::Transaction tx)
    : _tx(tx)
{
}

//==============================================================================

OnChainTx::OnChainTx(AssetID assetID, QString txId, QString blockHash, int64_t blockHeight,
    uint32_t txIndex, QDateTime timestamp, std::vector<chain::TxOutpoint> inputs,
    std::vector<chain::TxOutput> outputs, chain::OnChainTransaction::TxType type, TxMemo memo)
    : BaseTransaction(assetID)
{
    auto& where = tx();
    where.set_id(txId.toStdString());
    where.set_block_hash(blockHash.toStdString());
    where.set_block_height(blockHeight);
    where.set_index(txIndex);
    where.set_timestamp(timestamp.toMSecsSinceEpoch());
    where.set_type(type);

    auto& memos = *where.mutable_memo();

    for (auto&& it : memo) {
        memos.insert({ it.first, it.second });
    }

    for (auto&& input : inputs) {
        *where.add_inputs() = input;
    }

    for (auto&& output : outputs) {
        *where.add_outputs() = output;
    }
}

//==============================================================================

OnChainTx::OnChainTx(chain::Transaction tx)
    : BaseTransaction(tx)
{
}

//==============================================================================

void OnChainTx::invalidateBlockHeight()
{
    tx().set_block_height(-1);
    tx().set_index(0);
}

//==============================================================================

void OnChainTx::invalidateLocalCache(std::function<chain::TxOutput(chain::TxOutpoint)> outputById)
{
    for (auto out : tx().outputs()) {
        _delta += out.value();
        _unspentAddresses.insert(QString::fromStdString(out.address()));
    }

    for (auto in : tx().inputs()) {
        _delta -= outputById(in).value();
        _spendAddresses.insert(QString::fromStdString(outputById(in).address()));
    }
}

//==============================================================================

AddressesSet OnChainTx::getAddresses() const
{
    AddressesSet result;
    std::copy(std::begin(_spendAddresses), std::end(_spendAddresses),
        std::inserter(result, std::begin(result)));
    std::copy(std::begin(_unspentAddresses), std::end(_unspentAddresses),
        std::inserter(result, std::begin(result)));
    return result;
}

//==============================================================================

LightningPayment::LightningPayment(chain::Transaction tx)
    : BaseTransaction(tx)
{
}

//==============================================================================

TxMemo LightningPayment::memo() const
{
    TxMemo result;
    for (auto&& it : tx().memo()) {
        result.emplace(it.first, it.second);
    }

    return result;
}

//==============================================================================

LightningInvoice::LightningInvoice(chain::Transaction tx)
    : BaseTransaction(tx)
{
}

//==============================================================================

TxMemo LightningInvoice::memo() const
{
    TxMemo result;
    for (auto&& it : tx().memo()) {
        result.emplace(it.first, it.second);
    }

    return result;
}

//==============================================================================

LightningInvoiceRef TransactionUtils::FindInvoice(
    const LightningInvoiceList& where, size_t addIndex)
{
    auto it = std::lower_bound(std::begin(where), std::end(where), addIndex,
        [](const auto& lhs, const auto& value) { return lhs->addIndex() < value; });

    return it != std::end(where) && (*it)->addIndex() == addIndex ? *it : nullptr;
}

//==============================================================================

EthOnChainTxRef TransactionUtils::FindEthTransaction(
    const EthOnChainTxList& where, QString transactionID)
{
    auto it = std::find_if(std::begin(where), std::end(where),
        [transactionID](const auto& entry) { return entry->txId() == transactionID; });

    return it != std::end(where) ? *it : EthOnChainTxRef();
}

//==============================================================================

EthOnChainTx::EthOnChainTx(chain::Transaction tx)
    : BaseTransaction(tx)
{
    invalidateLocalCache();
}

//==============================================================================

EthOnChainTx::EthOnChainTx(AssetID assetID, QString txId, QString blockHash, int64_t blockHeight,
    eth::u256 gasUsed, eth::u256 gasPrice, eth::u256 value, std::string input, int64_t nonce,
    QString from, QString to, QDateTime timestamp, TxMemo memo)
    : BaseTransaction(assetID)
{
    auto& where = tx();
    where.set_id(txId.toStdString());
    where.set_block_hash(eth::FromHex(blockHash).toStdString());
    where.set_block_height(blockHeight);
    where.set_gasused(eth::ExportBitsData(gasUsed));
    where.set_gasprice(eth::ExportBitsData(gasPrice));
    where.set_value(eth::ExportBitsData(value));
    where.set_timestamp(timestamp.toMSecsSinceEpoch());
    where.set_input(input);
    where.set_nonce(nonce);
    where.set_to(eth::FromHex(to).toStdString());
    where.set_from(eth::FromHex(from).toStdString());

    auto& memos = *where.mutable_memo();

    for (auto&& it : memo) {
        memos.insert({ it.first, it.second });
    }

    invalidateLocalCache();
}

//==============================================================================

eth::u256 EthOnChainTx::gasUsed() const
{
    eth::u256 gasUsed;
    boost::multiprecision::import_bits(gasUsed, tx().gasused().begin(), tx().gasused().end(), 8);
    return gasUsed;
}

//==============================================================================

eth::u256 EthOnChainTx::gasPrice() const
{
    eth::u256 gasPrice;
    boost::multiprecision::import_bits(gasPrice, tx().gasprice().begin(), tx().gasprice().end(), 8);
    return gasPrice;
}

//==============================================================================

eth::u256 EthOnChainTx::value() const
{
    eth::u256 value;
    boost::multiprecision::import_bits(value, tx().value().begin(), tx().value().end(), 8);
    return value;
}
//==============================================================================

QString EthOnChainTx::from() const
{
    return eth::ChecksumAddress(QByteArray::fromStdString(tx().from()));
}

//==============================================================================

QString EthOnChainTx::to() const
{
    return eth::ChecksumAddress(QByteArray::fromStdString(tx().to()));
}

//==============================================================================

void EthOnChainTx::invalidateLocalCache()
{
    switch (tx().type()) {
    case chain::EthOnChainTransaction::EthTxType::EthOnChainTransaction_EthTxType_SEND_TX_TYPE: {
        _delta = isBaseChainTx() ? -eth::ConvertFromWeiToSats(value()) : -tokenAmount();
        break;
    }
    case chain::EthOnChainTransaction::EthTxType::EthOnChainTransaction_EthTxType_RECV_TX_TYPE: {
        _delta = isBaseChainTx() ? eth::ConvertFromWeiToSats(value()) : tokenAmount();
        break;
    }
    case chain::EthOnChainTransaction::EthTxType::
        EthOnChainTransaction_EthTxType_PAYMENT_TO_MYSELF_TX_TYPE: {
        _delta = -eth::ConvertFromWeiToSats(gasUsed());
        break;
    }
    default:
        break;
    }
}

//==============================================================================

void EthOnChainTx::updateTxType(chain::EthOnChainTransaction::EthTxType type)
{
    tx().set_type(type);
    invalidateLocalCache();
}

//==============================================================================

void EthOnChainTx::updateTxStatus(eth::u256 status)
{
    _isFailed = status == 0;
}

//==============================================================================

bool EthOnChainTx::isBaseChainTx()
{
    return tx().input() == "0x" || tx().input() == "";
}

//==============================================================================

qint64 EthOnChainTx::tokenAmount()
{
    if (!isBaseChainTx()) {
        return eth::ConvertDenominations(
            eth::ConvertFromHex(QString::fromStdString(tx().input()).right(64)),
            static_cast<uint32_t>(QString::fromStdString(tx().memo().at("decimals")).toUInt()), 8)
            .convert_to<int64_t>();
    }
    return 0;
}

//==============================================================================

TxMemo EthOnChainTx::memo() const
{
    TxMemo result;
    for (auto&& it : tx().memo()) {
        result.emplace(it.first, it.second);
    }

    return result;
}

//==============================================================================

EthOnChainTxRef TransactionUtils::UpdateEthTransactionType(
    QString ourAddress, bool isTransferPayload, EthOnChainTxRef tx)
{
    if (eth::CompareAddress(ourAddress, tx->from())) {
        if (eth::CompareAddress(ourAddress, tx->to())) {
            tx->updateTxType(chain::EthOnChainTransaction::EthTxType::
                    EthOnChainTransaction_EthTxType_PAYMENT_TO_MYSELF_TX_TYPE);
        } else {
            tx->updateTxType(chain::EthOnChainTransaction::EthTxType::
                    EthOnChainTransaction_EthTxType_SEND_TX_TYPE);
        }
    } else if (eth::CompareAddress(ourAddress, tx->to()) || isTransferPayload) {
        tx->updateTxType(
            chain::EthOnChainTransaction::EthTxType::EthOnChainTransaction_EthTxType_RECV_TX_TYPE);
    }

    return tx;
}

//==============================================================================

EthOnChainTxRef TransactionUtils::UpdateEthTransactionStatus(EthOnChainTxRef tx, QString status)
{
    tx->updateTxStatus(eth::ConvertFromHex(status));
    return tx;
}

//==============================================================================

ConnextPayment::ConnextPayment(chain::Transaction tx)
    : BaseTransaction(tx)
{
}

//==============================================================================

ConnextPayment::ConnextPayment(AssetID assetID, QString transferId, eth::u256 value,
    chain::ConnextPayment::ConnextPaymentType type, QString channelAddress, QDateTime timestamp,
    TxMemo memo)
    : BaseTransaction(assetID)
{
    auto& where = tx();
    where.set_transferid(transferId.toStdString());
    where.set_value(eth::ExportBitsData(value));
    where.set_timestamp(timestamp.toMSecsSinceEpoch());
    where.set_channeladdress(channelAddress.toStdString());
    where.set_type(type);

    auto& memos = *where.mutable_memo();

    for (auto&& it : memo) {
        memos.insert({ it.first, it.second });
    }
}

//==============================================================================

eth::u256 ConnextPayment::value() const
{
    eth::u256 value;
    boost::multiprecision::import_bits(value, tx().value().begin(), tx().value().end(), 8);
    return value;
}

//==============================================================================

qint64 ConnextPayment::delta() const
{
    auto txValue = eth::ConvertFromWeiToSats(value(),
        static_cast<uint32_t>(QString::fromStdString(tx().memo().at("decimals")).toUInt()), 8);
    return tx().type()
            == chain::ConnextPayment::ConnextPaymentType::ConnextPayment_ConnextPaymentType_SEND
        ? -txValue
        : txValue;
}

//==============================================================================

TxMemo ConnextPayment::memo() const
{
    TxMemo result;
    for (auto&& it : tx().memo()) {
        result.emplace(it.first, it.second);
    }

    return result;
}

//==============================================================================

ConnextPaymentRef TransactionUtils::FindConnextPaymentTransaction(
    const ConnextPaymentList& where, QString transferId)
{
    auto it = std::find_if(std::begin(where), std::end(where),
        [transferId](const auto& entry) { return entry->transferId() == transferId; });

    return it != std::end(where) ? *it : ConnextPaymentRef();
}

//==============================================================================
