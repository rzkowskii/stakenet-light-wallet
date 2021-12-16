#include "WalletTransactionsListModel.hpp"
#include <Chain/AbstractChainManager.hpp>
#include <Chain/Chain.hpp>
#include <Data/TransactionEntry.hpp>
#include <Data/WalletAssetsModel.hpp>
#include <Models/AssetTransactionsDataSource.hpp>
#include <Tools/Common.hpp>
#include <Utils/Logging.hpp>

#include <QVector>
#include <cmath>

//==============================================================================

static QVariantList GetAddresses(const OnChainTx& transaction)
{
    QVariantList result;
    auto addresses = transaction.getAddresses();
    std::transform(std::begin(addresses), std::end(addresses), std::back_inserter(result),
        [](const auto address) { return QVariant::fromValue(address); });

    return result;
}

//==============================================================================

static QString GetLightningTxType(lndtypes::LightningPaymentReason type)
{
    switch (type) {
    case lndtypes::LightningPaymentReason::PAYMENT_RENTAL:
        return "channel rental";
    case lndtypes::LightningPaymentReason::PAYMENT_PAYMENT:
    case lndtypes::LightningPaymentReason::UNKNOWN_PAYMENT_REASON:
        return "sent";
    case lndtypes::LightningPaymentReason::PAYMENT_SWAP_PAYMENT:
        return "dex sent";
    case lndtypes::LightningPaymentReason::PAYMENT_ORDERBOOK_FEE:
        return "fee";
    case lndtypes::LightningPaymentReason::PAYMENT_RENTAL_EXTENSION:
        return "rental extension";

    default:
        Q_ASSERT(false);
    }

    return QString();
}

//==============================================================================

static bool IsLightningTxDexType(lndtypes::LightningPaymentReason type)
{
    switch (type) {
    case lndtypes::LightningPaymentReason::PAYMENT_RENTAL:
    case lndtypes::LightningPaymentReason::PAYMENT_PAYMENT:
    case lndtypes::LightningPaymentReason::UNKNOWN_PAYMENT_REASON:
    case lndtypes::LightningPaymentReason::PAYMENT_RENTAL_EXTENSION:
        return false;
    case lndtypes::LightningPaymentReason::PAYMENT_SWAP_PAYMENT:
    case lndtypes::LightningPaymentReason::PAYMENT_ORDERBOOK_FEE:
        return true;

    default:
        Q_ASSERT(false);
    }

    return false;
}

//==============================================================================

static QString GetLightningInvoiceType(lndtypes::LightingInvoiceReason type)
{
    switch (type) {
    case lndtypes::LightingInvoiceReason::INVOICE_PAYMENT:
    case lndtypes::LightingInvoiceReason::UNKNOWN_INVOICE_REASON:
        return "invoice";
    case lndtypes::LightingInvoiceReason::INVOICE_REFUND_FEE:
        return "fee refund";
    case lndtypes::LightingInvoiceReason::INVOICE_SWAP_PAYMENT:
        return "dex received";

    default:
        Q_ASSERT(false);
    }

    return QString();
}

//==============================================================================

static QString GetConnextPaymentTxType(chain::ConnextPayment::ConnextPaymentType type)
{
    switch (type) {
    case chain::ConnextPayment::ConnextPaymentType::ConnextPayment_ConnextPaymentType_SEND:
        return "dex send";
    case chain::ConnextPayment::ConnextPaymentType::ConnextPayment_ConnextPaymentType_RECEIVE:
        return "dex receive";
    default:
        Q_ASSERT(false);
    }

    return QString();
}
//==============================================================================

static bool IsLightninginvoiceDexType(lndtypes::LightingInvoiceReason type)
{
    switch (type) {
    case lndtypes::LightingInvoiceReason::INVOICE_PAYMENT:
    case lndtypes::LightingInvoiceReason::UNKNOWN_INVOICE_REASON:
        return false;
    case lndtypes::LightingInvoiceReason::INVOICE_REFUND_FEE:
    case lndtypes::LightingInvoiceReason::INVOICE_SWAP_PAYMENT:
        return true;

    default:
        Q_ASSERT(false);
    }

    return false;
}

//==============================================================================

static QString GetLightningTxStatus(chain::LightningPayment::PaymentStatus status)
{
    switch (status) {
    case chain::LightningPayment::PaymentStatus::LightningPayment_PaymentStatus_SUCCEEDED:
        return "success";
    case chain::LightningPayment::PaymentStatus::LightningPayment_PaymentStatus_IN_FLIGHT:
        return "in flight";
    case chain::LightningPayment::PaymentStatus::LightningPayment_PaymentStatus_FAILED:
        return "failed";
    case chain::LightningPayment::PaymentStatus::LightningPayment_PaymentStatus_UNKNOWN:
        return "unknown";

    default:
        Q_ASSERT(false);
    }

    return QString();
}

//==============================================================================

static QString GetLnTxStatusColor(chain::LightningPayment::PaymentStatus type)
{
    switch (type) {
    case chain::LightningPayment::PaymentStatus::LightningPayment_PaymentStatus_SUCCEEDED:
        return "#3FBE4D";
    case chain::LightningPayment::PaymentStatus::LightningPayment_PaymentStatus_IN_FLIGHT:
        return "#393E43";
    case chain::LightningPayment::PaymentStatus::LightningPayment_PaymentStatus_FAILED:
        return "#E2344F";
    case chain::LightningPayment::PaymentStatus::LightningPayment_PaymentStatus_UNKNOWN:
        return "#393E43";

    default:
        Q_ASSERT(false);
    }

    return QString();
}

//==============================================================================

static QString GetLightningInvoiceState(chain::LightningInvoice::InvoiceState state)
{
    switch (state) {
    case chain::LightningInvoice::InvoiceState::LightningInvoice_InvoiceState_SETTLED:
        return "received";
    case chain::LightningInvoice::InvoiceState::LightningInvoice_InvoiceState_ACCEPTED:
        return "accepted";
    case chain::LightningInvoice::InvoiceState::LightningInvoice_InvoiceState_OPEN:
        return "pending";
    case chain::LightningInvoice::InvoiceState::LightningInvoice_InvoiceState_CANCELED:
        return "cancelled";

    default:
        Q_ASSERT(false);
    }

    return QString();
}

//==============================================================================

static QString getInvoiceTxStateColor(chain::LightningInvoice::InvoiceState state)
{
    switch (state) {
    case chain::LightningInvoice::InvoiceState::LightningInvoice_InvoiceState_SETTLED:
        return "#3FBE4D";
    case chain::LightningInvoice::InvoiceState::LightningInvoice_InvoiceState_ACCEPTED:
        return "#393E43";
    case chain::LightningInvoice::InvoiceState::LightningInvoice_InvoiceState_OPEN:
        return "#393E43";
    case chain::LightningInvoice::InvoiceState::LightningInvoice_InvoiceState_CANCELED:
        return "#E2344F";

    default:
        Q_ASSERT(false);
    }

    return QString();
}

//==============================================================================

static QString getEthTxStatusColor(bool isConfirmed, bool isFailed)
{
    if (isFailed) {
        return "#E2344F";
    }
    return isConfirmed ? QString("#3FBE4D") : QString("#F09F1E");
}

//==============================================================================

static QString GetStatus(
    bool conflicted, int confirmations, int confirmationsForApproved, bool isFailed = false)
{
    return conflicted
        ? "conflicted"
        : (isFailed ? "failed"
                    : (confirmations >= confirmationsForApproved ? "success" : "pending"));
}
//==============================================================================

WalletTransactionsListModel::WalletTransactionsListModel(
    QPointer<AssetTransactionsDataSource> dataSource, QPointer<WalletAssetsModel> walletAssetsModel,
    QPointer<AbstractChainManager> chainManager, QObject* parent)
    : QAbstractListModel(parent)
    , _dataSource(dataSource)
    , _walletAssetsModel(walletAssetsModel)
    , _chainManager(chainManager)
{
    init();
}

//==============================================================================

WalletTransactionsListModel::~WalletTransactionsListModel() {}

//==============================================================================

int WalletTransactionsListModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return static_cast<int>(_rowCount);
}

//==============================================================================

QVariant WalletTransactionsListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    struct Visitor {
        Visitor(const WalletTransactionsListModel* self, int role)
            : self(self)
            , role(role)
        {
        }

        QVariant operator()(const OnChainTxRef& tx) const
        {
            const CoinAsset coin = self->_walletAssetsModel->assetById(tx->assetID());
            const int confirmations = self->getConfirmations(*tx);
            const int confirmationsForApproved
                = static_cast<int>(coin.misc().confirmationsForApproved);

            switch (role) {
            case TransactionTypeRole:
                return static_cast<int>(TransactionType::OnChain);
            case TransactionIDRole:
                return tx->txId();
            case DeltaRole:
                return FormatAmount(tx->delta());
            case TxDateRole:
                return tx->transactionDate().toMSecsSinceEpoch();
            case TxAmountRole:
                return tx->delta();
            case CurrencyRole:
                return coin.name();
            case SymbolRole:
                return coin.ticket();
            case StatusRole:
                return GetStatus(tx->isConflicted(), confirmations, confirmationsForApproved);
            case ActivityRole:
                return self->getOnChainTxActivity(*tx);
            case IsDexTxRole:
                return false;
            case TxDataRole:
                QVariantMap result;
                result["txId"] = tx->txId();
                result["txImg"] = tx->delta() > 0 ? QString("qrc:/images/RECEIVE_MONEY.png")
                                                  : QString("qrc:/images/SEND_MONEY.png");
                result["statusColor"] = confirmations >= confirmationsForApproved
                    ? QString("#3FBE4D")
                    : QString("#F09F1E");
                result["confirmations"] = confirmations;
                result["confirmationsForApproved"] = confirmationsForApproved;
                result["txAmount"] = tx->delta();
                result["txDate"] = tx->transactionDate().toMSecsSinceEpoch();
                result["delta"] = FormatAmount(tx->delta());
                result["fee"] = FormatAmount(tx->fee());
                result["status"]
                    = GetStatus(tx->isConflicted(), confirmations, confirmationsForApproved);
                result["addresses"] = GetAddresses(*tx);
                return result;
            }

            return QVariant();
        }

        QVariant operator()(const LightningInvoiceRef& tx) const
        {
            const CoinAsset coin = self->_walletAssetsModel->assetById(tx->assetID());

            switch (role) {
            case TransactionTypeRole:
                return static_cast<int>(TransactionType::LightningInvoice);
            case TransactionIDRole:
                return tx->rHash();
            case DeltaRole:
                return FormatAmount(tx->value());
            case TxDateRole:
                return tx->transactionDate().toMSecsSinceEpoch();
            case TxAmountRole:
                return QVariant::fromValue(tx->value());
            case CurrencyRole:
                return coin.name();
            case SymbolRole:
                return coin.ticket();
            case StatusRole:
                return GetLightningInvoiceState(tx->state());
            case ActivityRole:
                return GetLightningInvoiceType(tx->type());
            case IsDexTxRole:
                return IsLightninginvoiceDexType(tx->type());
            case TxDataRole:
                QVariantMap result;
                result["txId"] = tx->rHash();
                result["txImg"] = QString("qrc:/images/LIGHTNING_ACTIVE.png");
                result["statusColor"] = getInvoiceTxStateColor(tx->state());
                result["txAmount"] = QVariant::fromValue(tx->value());
                result["txDate"] = tx->transactionDate().toMSecsSinceEpoch();
                result["delta"] = FormatAmount(tx->value());
                result["status"] = GetLightningInvoiceState(tx->state());
                result["txActivity"] = GetLightningInvoiceType(tx->type());

                return result;
            }

            return QVariant();
        }

        QVariant operator()(const LightningPaymentRef& tx) const
        {
            const CoinAsset coin = self->_walletAssetsModel->assetById(tx->assetID());

            switch (role) {
            case TransactionTypeRole:
                return static_cast<int>(TransactionType::LightningPayment);
            case TransactionIDRole:
                return tx->paymentHash();
            case DeltaRole:
                return FormatAmount(tx->value());
            case TxDateRole:
                return tx->transactionDate().toMSecsSinceEpoch();
            case TxAmountRole:
                return tx->value() * -1;
            case CurrencyRole:
                return coin.name();
            case SymbolRole:
                return coin.ticket();
            case StatusRole:
                return GetLightningTxStatus(tx->status());
            case ActivityRole:
                return GetLightningTxType(tx->type());
            case IsDexTxRole:
                return IsLightningTxDexType(tx->type());
            case TxDataRole:
                QVariantMap result;
                result["txId"] = tx->paymentHash();
                result["txImg"] = QString("qrc:/images/LIGHTNING_ACTIVE.png");
                result["txDate"] = tx->transactionDate().toMSecsSinceEpoch();
                result["txAmount"] = QVariant::fromValue(tx->value() * -1);
                result["delta"] = FormatAmount(tx->value());
                result["status"] = GetLightningTxStatus(tx->status());
                result["statusColor"] = GetLnTxStatusColor(tx->status());
                result["txActivity"] = GetLightningTxType(tx->type());
                return result;
            }

            return QVariant();
        }

        QVariant operator()(const EthOnChainTxRef& tx)
        {
            const CoinAsset coin = self->_walletAssetsModel->assetById(tx->assetID());
            const int confirmations = self->getConfirmations(*tx);
            const int confirmationsForApproved
                = static_cast<int>(coin.misc().confirmationsForApproved);

            auto fee = eth::u256{ (tx->gasUsed() * tx->gasPrice()) };

            switch (role) {
            case TransactionTypeRole:
                return static_cast<int>(TransactionType::EthOnChainTx);
            case TransactionIDRole:
                return tx->txId();
            case DeltaRole:
                return FormatAmount(tx->delta());
            case TxDateRole:
                return tx->transactionDate().toMSecsSinceEpoch();
            case TxAmountRole:
                return tx->delta();
            case CurrencyRole:
                return coin.name();
            case SymbolRole:
                return coin.ticket();
            case StatusRole:
                return GetStatus(
                    tx->isConflicted(), confirmations, confirmationsForApproved, tx->isFailed());
            case ActivityRole:
                return self->getOnChainTxActivity(*tx);
                ;
            case IsDexTxRole:
                return false;
            case TxDataRole:
                QVariantMap result;
                result["txId"] = tx->txId();
                result["txImg"] = tx->type()
                        == chain::EthOnChainTransaction::EthTxType::
                               EthOnChainTransaction_EthTxType_RECV_TX_TYPE
                    ? QString("qrc:/images/RECEIVE_MONEY.png")
                    : QString("qrc:/images/SEND_MONEY.png");
                result["statusColor"] = getEthTxStatusColor(
                    confirmations >= confirmationsForApproved, tx->isFailed());
                result["confirmations"] = confirmations;
                result["confirmationsForApproved"] = confirmationsForApproved;
                result["txAmount"] = tx->delta();
                result["txDate"] = tx->transactionDate().toMSecsSinceEpoch();
                result["delta"] = FormatAmount(tx->delta());
                result["fee"]
                    = FormatAmount(ConvertFromWeiToSats(QString::fromStdString(fee.str())));
                result["status"] = GetStatus(
                    tx->isConflicted(), confirmations, confirmationsForApproved, tx->isFailed());
                result["addresses"] = self->getAddresses(*tx);
                result["nonce"] = QString::number(tx->nonce());
                result["gasPrice"] = FormatAmount(
                    ConvertFromWeiToSats(QString::fromStdString(tx->gasPrice().str())));
                result["blockHash"] = tx->blockHash();
                result["blockHeight"] = QString::number(tx->blockHeight());
                return result;
            }

            return QVariant();
        }

        QVariant operator()(const ConnextPaymentRef& tx) {
            const CoinAsset coin = self->_walletAssetsModel->assetById(tx->assetID());

            switch (role) {
            case TransactionTypeRole:
                return static_cast<int>(TransactionType::LightningPayment);
            case TransactionIDRole:
                return tx->transferId();
            case DeltaRole:
                return FormatAmount(tx->delta());
            case TxDateRole:
                return tx->transactionDate().toMSecsSinceEpoch();
            case TxAmountRole:
                return tx->delta();
            case CurrencyRole:
                return coin.name();
            case SymbolRole:
                return coin.ticket();
            case StatusRole:
                return "success";//tx->status(); //TODO status and status color
            case ActivityRole:
                return GetConnextPaymentTxType(tx->type());
            case IsDexTxRole:
                return true;//IsLightningTxDexType(tx->type());
            case TxDataRole:
                QVariantMap result;
                result["txId"] = tx->transferId();
                result["txImg"] = QString("qrc:/images/LIGHTNING_ACTIVE.png");
                result["txDate"] = tx->transactionDate().toMSecsSinceEpoch();
                result["txAmount"] = tx->delta();
                result["delta"] = FormatAmount(tx->delta());
                result["status"] = "success";//tx->status();;
                result["statusColor"] = "#3FBE4D";//GetLnTxStatusColor(tx->status());
                result["txActivity"] = GetConnextPaymentTxType(tx->type());
                return result;
            }

            return QVariant();
        }
        const WalletTransactionsListModel* self;
        int role;
    };

    const size_t row = static_cast<size_t>(index.row());
    auto transaction = _dataSource->transactionsList().at(row);

    return boost::variant2::visit(Visitor(this, role), transaction);
}

//==============================================================================

QHash<int, QByteArray> WalletTransactionsListModel::roleNames() const
{
    static QHash<int, QByteArray> roleNames;
    if (roleNames.isEmpty()) {
        roleNames[Roles::TransactionTypeRole] = "txType";
        roleNames[Roles::TransactionIDRole] = "id";
        roleNames[Roles::DeltaRole] = "delta";
        roleNames[Roles::TxDateRole] = "txDate";
        roleNames[Roles::CurrencyRole] = "currency";
        roleNames[Roles::SymbolRole] = "symbol";
        roleNames[Roles::TxAmountRole] = "txAmount";
        roleNames[Roles::StatusRole] = "status";
        roleNames[Roles::ActivityRole] = "activity";
        roleNames[Roles::TxDataRole] = "txData";
        roleNames[Roles::IsDexTxRole] = "isDexTx";
    }
    return roleNames;
}

//==============================================================================

void WalletTransactionsListModel::onTransactionsFetched()
{
    updateConfirmations();
    beginResetModel();
    _rowCount = _dataSource->transactionsList().size();
    endResetModel();
}

//==============================================================================

void WalletTransactionsListModel::onTxnsAdded(const std::vector<Transaction>& txns)
{
    Q_UNUSED(txns);
    int rows = rowCount(QModelIndex());
    beginInsertRows(QModelIndex(), rows, rows + txns.size() - 1);
    _rowCount += txns.size();
    endInsertRows();
}

//==============================================================================

void WalletTransactionsListModel::onTxnsChanged(
    const std::vector<Transaction>& txns, std::vector<int> indexes)
{
    Q_UNUSED(txns)
    for (auto i : indexes) {
        auto row = index(i);
        dataChanged(row, row);
    }
}

//==============================================================================

void WalletTransactionsListModel::updateConfirmations()
{
    if (_chainView) {
        _chainView->chainHeight().then([=](size_t newHeight) {
            _tipHeight = newHeight;
            dataChanged(index(0), index(rowCount(QModelIndex()) - 1));
        });
    }
}

//==============================================================================

void WalletTransactionsListModel::onChainsLoaded()
{
    if (_chainView) {
        return;
    }

    _chainManager
        ->getChainView(
            _walletAssetsModel->assetById(_dataSource->assetID()).params().params.extCoinType(),
            AbstractChainManager::ChainViewUpdatePolicy::CompressedEvents)
        .then([=](std::shared_ptr<ChainView> chainView) {
            _chainView.swap(chainView);
            connect(_chainView.get(), &ChainView::bestBlockHashChanged, this,
                &WalletTransactionsListModel::updateConfirmations);
            updateConfirmations();
        });
}

//==============================================================================

void WalletTransactionsListModel::init()
{
    if (_dataSource) {
        connect(_dataSource, &AssetTransactionsDataSource::transactionsFetched, this,
            &WalletTransactionsListModel::onTransactionsFetched);
        connect(_dataSource, &AssetTransactionsDataSource::txnsAdded, this,
            &WalletTransactionsListModel::onTxnsAdded);
        connect(_dataSource, &AssetTransactionsDataSource::txnsChanged, this,
            &WalletTransactionsListModel::onTxnsChanged);
        connect(_chainManager, &AbstractChainManager::chainsLoaded, this,
            &WalletTransactionsListModel::onChainsLoaded);
        onChainsLoaded();
        _dataSource->fetchTransactions();
    }
}

//==============================================================================

QVariantList WalletTransactionsListModel::getAddresses(const OnChainTx& transaction) const
{
    QVariantList result;
    auto addresses = transaction.getAddresses();
    std::transform(std::begin(addresses), std::end(addresses), std::back_inserter(result),
        [](const auto address) { return QVariant::fromValue(address); });

    return result;
}

//==============================================================================

int WalletTransactionsListModel::getConfirmations(const OnChainTx& transaction) const
{
    return _tipHeight > 0 && transaction.blockHeight() > 0
        ? (_tipHeight - transaction.blockHeight() + 1)
        : 0;
}

//==============================================================================

int WalletTransactionsListModel::getConfirmations(const EthOnChainTx& transaction) const
{
    return _tipHeight > 0 && transaction.blockHeight() > 0
        ? (_tipHeight - transaction.blockHeight() + 1)
        : 0;
}

//==============================================================================

static Enums::TransactionFlags ExtractFlag(const OnChainTx& tx)
{
    if (tx.tx().memo().count(OnChainTx::MEMO_TX_TYPE_FLAG)) {
        try {
            return static_cast<Enums::TransactionFlags>(
                std::stoi(tx.tx().memo().at(OnChainTx::MEMO_TX_TYPE_FLAG)));
        } catch (...) {
        }
    }

    return Enums::TransactionFlags::NoFlags;
}

//==============================================================================

QString WalletTransactionsListModel::getOnChainTxActivity(const OnChainTx& transaction) const
{
    switch (transaction.type()) {
    case chain::OnChainTransaction::TxType::OnChainTransaction_TxType_PAYMENT: {

        if (ExtractFlag(transaction) == Enums::TransactionFlags::PaymentToMyself) {
            return tr("Payment to myself");
        }

        return transaction.delta() > 0 ? tr("received") : tr("sent");
    }
    case chain::OnChainTransaction::TxType::OnChainTransaction_TxType_OPEN_CHANNEL:
        return tr("Open channel");
    case chain::OnChainTransaction::TxType::OnChainTransaction_TxType_CLOSE_CHANNEL:
        return tr("Close channel");

    default:
        Q_ASSERT(false);
    }

    return QString();
}

//==============================================================================

QString WalletTransactionsListModel::getOnChainTxActivity(const EthOnChainTx& transaction) const
{
    // TODO: check why payment to self is marked as sent
    switch (transaction.type()) {
    case chain::EthOnChainTransaction::EthTxType::EthOnChainTransaction_EthTxType_SEND_TX_TYPE:
        return tr("sent");
    case chain::EthOnChainTransaction::EthTxType::EthOnChainTransaction_EthTxType_RECV_TX_TYPE:
        return tr("received");
    case chain::EthOnChainTransaction::EthTxType::
        EthOnChainTransaction_EthTxType_PAYMENT_TO_MYSELF_TX_TYPE:
        return tr("Payment to myself");

    default:
        Q_ASSERT(false);
    }

    return QString();
}

//==============================================================================

QVariantList WalletTransactionsListModel::getAddresses(const EthOnChainTx& transaction) const
{
    QVariantList result;
    result.push_back(QVariant::fromValue(transaction.to()));
    result.push_back(QVariant::fromValue(transaction.from()));
    return result;
}

//==============================================================================
