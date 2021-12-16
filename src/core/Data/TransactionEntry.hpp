#ifndef TRANSACTIONENTRY_HPP
#define TRANSACTIONENTRY_HPP

#include <Chain/Protos/Chain.pb.h>
#include <EthCore/Types.hpp>
#include <Tools/Common.hpp>

#include <QDateTime>
#include <QObject>
#include <QPointer>
#include <QString>
#include <boost/optional.hpp>
#include <boost/variant2/variant.hpp>
#include <map>
#include <memory>
#include <vector>

class WalletAssetsModel;

//==============================================================================

using TxMemo = std::map<std::string, std::string>;

//==============================================================================

struct BaseTransaction {
    explicit BaseTransaction(AssetID assetID);
    explicit BaseTransaction(chain::Transaction tx);

    AssetID assetID() const { return _tx.asset_id(); }

    chain::Transaction _tx;
};

//==============================================================================

struct OnChainTx : BaseTransaction {
    using Inputs = std::vector<chain::TxOutpoint>;
    using Outputs = std::vector<chain::TxOutput>;
    explicit OnChainTx(chain::Transaction tx);

    OnChainTx(AssetID assetID, QString txId, QString blockHash, int64_t blockHeight,
        uint32_t txIndex, QDateTime timestamp, Inputs inputs, Outputs outputs,
        chain::OnChainTransaction::TxType type, TxMemo memo);

    chain::OnChainTransaction& tx() { return *_tx.mutable_onchain_tx(); }
    const chain::OnChainTransaction& tx() const { return _tx.onchain_tx(); }

    QString txId() const { return QString::fromStdString(tx().id()); }
    QString blockHash() const { return QString::fromStdString(tx().block_hash()); }
    int64_t blockHeight() const { return tx().block_height(); }
    uint32_t transactionIndex() const { return tx().index(); }
    qint64 delta() const { return _delta; }
    qint64 fee() const { return tx().fee(); }
    QDateTime transactionDate() const { return QDateTime::fromMSecsSinceEpoch(tx().timestamp()); }
    QString serialized() const { return QString::fromStdString(tx().hexrawtransaction()); }

    chain::OnChainTransaction::TxType type() const { return tx().type(); }

    void setHexSerialization(std::string serialization)
    {
        tx().set_hexrawtransaction(serialization);
    }

    const google::protobuf::RepeatedPtrField<chain::TxOutpoint>& inputs() const
    {
        return tx().inputs();
    }
    const google::protobuf::RepeatedPtrField<chain::TxOutput>& outputs() const
    {
        return tx().outputs();
    }

    bool isConflicted() const { return !tx().block_hash().empty() && tx().block_height() < 0; }

    void invalidateBlockHeight();
    void invalidateLocalCache(std::function<chain::TxOutput(chain::TxOutpoint)> outputById);
    AddressesSet getAddresses() const;

    //    ConflictingInputs _conflictedInputs; // memonly cache
    qint64 _delta{ 0 }; // memonly cache
    AddressesSet _spendAddresses; // memonly cache
    AddressesSet _unspentAddresses; // memonly cache

    // transaction flags
    static std::string MEMO_TX_TYPE_FLAG;
};

//==============================================================================

struct EthOnChainTx : BaseTransaction {
    explicit EthOnChainTx(chain::Transaction tx);

    static const QString UNKNOWN_BLOCK_HASH;
    static const int64_t UNKNOWN_BLOCK_HEIGHT;
    static const int64_t UNKNOWN_GAS_USED;

    EthOnChainTx(AssetID assetID, QString txId, QString blockHash, int64_t blockHeight,
        eth::u256 gasUsed, eth::u256 gasPrice, eth::u256 value, std::string input, int64_t nonce,
        QString from, QString to, QDateTime timestamp, TxMemo memo);

    chain::EthOnChainTransaction& tx() { return *_tx.mutable_ethonchain_tx(); }
    const chain::EthOnChainTransaction& tx() const { return _tx.ethonchain_tx(); }

    QString txId() const { return QString::fromStdString(tx().id()); }
    QString blockHash() const { return QByteArray::fromStdString(tx().block_hash()).toHex(); }
    int64_t blockHeight() const { return tx().block_height(); }
    QDateTime transactionDate() const { return QDateTime::fromMSecsSinceEpoch(tx().timestamp()); }
    eth::u256 gasUsed() const;
    eth::u256 gasPrice() const;
    eth::u256 value() const;
    std::string input() const { return tx().input(); }
    int64_t nonce() const { return tx().nonce(); }
    /*!
     * \brief from returns address of sender, this could be a smart contract or user address
     * ATTENTION: this address can be in checksum format, to compare it use eth::CompareAddress
     * \return address of sender
     */
    QString from() const;
    /*!
     * \brief to returns address of receiver, this could be a smart contract or user address.
     * ATTENTION: this address can be in checksum format, to compare it use eth::CompareAddress
     * \return address of receiver
     */
    QString to() const;
    chain::EthOnChainTransaction::EthTxType type() const { return tx().type(); }
    qint64 delta() const { return _delta; }
    bool isFailed() const { return _isFailed; }

    bool isConflicted() const { return !tx().block_hash().empty() && tx().block_height() < 0; }
    void invalidateLocalCache();
    void updateTxType(chain::EthOnChainTransaction::EthTxType type);
    void updateTxStatus(eth::u256 status); // status: 0 - tx failed, 1 - tx success
    bool isBaseChainTx();
    qint64 tokenAmount();
    TxMemo memo() const;

    qint64 _delta{ 0 }; // memonly cache
    bool _isFailed{ false }; // memonly cache
};

//==============================================================================

struct LightningPayment : BaseTransaction {
    explicit LightningPayment(chain::Transaction tx);

    chain::LightningPayment& tx() { return *_tx.mutable_lightning_payment(); }
    const chain::LightningPayment& tx() const { return _tx.lightning_payment(); }

    uint64_t paymentIndex() { return tx().payment_index(); }
    QDateTime transactionDate() const { return QDateTime::fromSecsSinceEpoch(tx().timestamp()); }

    QString paymentHash() const { return QString::fromStdString(tx().payment_hash()); }
    qint64 value() const { return tx().value(); }

    chain::LightningPayment::PaymentStatus status() const { return tx().status(); }
    lndtypes::LightningPaymentReason type() const { return tx().type(); }
    TxMemo memo() const;
};

//==============================================================================

struct LightningInvoice : BaseTransaction {
    explicit LightningInvoice(chain::Transaction tx);

    chain::LightningInvoice& tx() { return *_tx.mutable_lightning_invoice(); }
    const chain::LightningInvoice& tx() const { return _tx.lightning_invoice(); }

    QString rHash() const { return QByteArray::fromStdString(tx().r_hash()).toHex(); }
    int64_t value() const { return tx().value(); }
    uint64_t addIndex() const { return tx().add_index(); }
    uint64_t settleIndex() const { return tx().settle_index(); }
    QDateTime transactionDate() const
    {
        return QDateTime::fromSecsSinceEpoch(tx().creation_timestamp());
    }
    QDateTime settleDate() const { return QDateTime::fromSecsSinceEpoch(tx().settle_timestamp()); }
    lndtypes::LightingInvoiceReason type() const { return tx().type(); }
    chain::LightningInvoice::InvoiceState state() const { return tx().state(); }
    TxMemo memo() const;
};

//==============================================================================

struct ConnextPayment : BaseTransaction {
    explicit ConnextPayment(chain::Transaction tx);

    ConnextPayment(AssetID assetID, QString transferId, eth::u256 value,
        chain::ConnextPayment::ConnextPaymentType type, QString channelAddress, QDateTime timestamp,
        TxMemo memo);

    chain::ConnextPayment& tx() { return *_tx.mutable_connext_payment(); }
    const chain::ConnextPayment& tx() const { return _tx.connext_payment(); }

    QDateTime transactionDate() const { return QDateTime::fromMSecsSinceEpoch(tx().timestamp()); }
    chain::ConnextPayment::ConnextPaymentType type() const { return tx().type(); }
    eth::u256 value() const;

    qint64 delta() const;

    QString transferId() const { return QString::fromStdString(tx().transferid()); }
    QString channelAddress() const { return QString::fromStdString(tx().channeladdress()); }

    TxMemo memo() const;
};

//==============================================================================

using OnChainTxRef = std::shared_ptr<OnChainTx>;
using OnChainTxList = std::vector<OnChainTxRef>;
using OnChainTxMap = std::map<AssetID, OnChainTxList>;
using EthOnChainTxRef = std::shared_ptr<EthOnChainTx>;
using EthOnChainTxList = std::vector<EthOnChainTxRef>;
using LightningPaymentRef = std::shared_ptr<LightningPayment>;
using LightningPaymentList = std::vector<LightningPaymentRef>;
using LightningInvoiceRef = std::shared_ptr<LightningInvoice>;
using LightningInvoiceList = std::vector<LightningInvoiceRef>;
using ConnextPaymentRef = std::shared_ptr<ConnextPayment>;
using ConnextPaymentList = std::vector<ConnextPaymentRef>;

using Transaction = boost::variant2::variant<OnChainTxRef, LightningPaymentRef, LightningInvoiceRef,
    EthOnChainTxRef, ConnextPaymentRef>;
using TransactionsList = std::vector<Transaction>;

// spend info about conflicting inputs, at which transaction outpoint was spent
// using ConflictingInputs = std::map<Outpoint, QString>;

//==============================================================================

QDebug operator<<(QDebug debug, const Transaction& transactionEntry);

//==============================================================================

class MutableTransaction {
public:
    explicit MutableTransaction(AssetID assetID);

    using Script = std::vector<unsigned char>;
    // TxIn - Outpoint hash, Outpoint n, scriptSig
    using TxIn = std::tuple<std::string, unsigned, Script>;
    // TxOut - scriptPubKey, amount
    using TxOut = std::tuple<Script, Balance>;

    const std::vector<TxIn>& inputs() const;
    const std::vector<TxOut>& outputs() const;

    void addInputs(std::vector<TxIn> vec);
    void addOutputs(std::vector<TxOut> vec);
    void setInput(size_t index, TxIn input);
    void setOutput(size_t index, TxOut output);

    void setHashAndHex(std::string hash, std::string txHex);

    bool isValid() const;

    std::string txId() const;
    std::string serialized() const;

    AssetID assetID() const;
    int changePos() const;
    void setChangePos(int changePos);

    qint64 fee() const;
    void setFee(qint64 fee);

    QString recipientAddress() const;
    void setRecipientAddress(QString recipientAddress);

    static MutableTransaction FromRawTx(AssetID assetID, std::string rawTx);

private:
    void invalidateHash();

private:
    AssetID _assetID;
    std::vector<TxIn> _inputs;
    std::vector<TxOut> _outputs;
    std::string _hash;
    std::string _serializedTransactionHex;
    bool _hashValid{ false };
    int _changePos;
    qint64 _fee;
    QString _recipientAddress;
};

//==============================================================================

using MutableTransactionRef = std::shared_ptr<MutableTransaction>;

//==============================================================================

namespace TransactionUtils {
OnChainTxRef FindTransaction(const OnChainTxList& where, QString transactionID);
boost::optional<chain::TxOutput> FindOutput(const OnChainTx& tx, size_t index);
OnChainTxRef MutableTxToTransactionRef(
    const MutableTransaction& transaction, const WalletAssetsModel& walletAssetsModel);
LightningInvoiceRef FindInvoice(const LightningInvoiceList& where, size_t addIndex);
EthOnChainTxRef FindEthTransaction(const EthOnChainTxList& where, QString transactionID);
EthOnChainTxRef UpdateEthTransactionType(QString ourAddress, bool isTokenTx, EthOnChainTxRef tx);
EthOnChainTxRef UpdateEthTransactionStatus(EthOnChainTxRef tx, QString status = "0x1");
ConnextPaymentRef FindConnextPaymentTransaction(const ConnextPaymentList &where, QString transferId);
}

//==============================================================================

#endif // TRANSACTIONENTRY_HPP
