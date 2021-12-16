#ifndef COMMON_HPP
#define COMMON_HPP
#include <QDir>
#include <QObject>
#include <QVector>
#include <QtPromise>
#include <array>
#include <optional>
#include <set>
#include <stdint.h>

struct CoinAsset;

using AssetID = uint32_t;
using AccountIndex = uint32_t;
using Balance = int64_t;
using Address = QString;
using TxID = QString;
using SyncEntry = std::tuple<Address, TxID, AssetID>;
using AddressesList = std::vector<Address>;
using AddressesSet = std::set<Address>;
using ChainHeight = unsigned;
using BlockHash = QString;
using BlockHeight = size_t;
using AddressEntry = std::pair<QList<QString>, unsigned>;
using DexAssetPair = std::pair<AssetID, AssetID>;

static std::map<AssetID, int64_t> UNITS_PER_CURRENCY = {
    { 0, 8 }, // BTC
    { 2, 8 }, // LTC
    { 60, 18 }, // ETH
    { 60001, 18 }, // Wrapped Ether WETH
    { 60002, 6 }, // USDT
    { 60003, 6 }, // USD coin
    { 9999, 18 }, // Ethereum Ropsten rETH
    { 9999001, 18 }, // Wrapped Ether Ropsten rETH
    { 88, 18 }, // Ethereum Rinkeby
    { 88001, 18 }, // Wrapped Ether Ropsten rETH
    { 384, 8 } }; // XSN
// { "DAI", 18 },
//{ "TTT", 18 },

static std::map<AssetID, std::vector<AssetID>> PAYING_CURRENCIES ={
    { 0, { 0, 2, 60, 384} }, // BTC
    { 2, { 0, 2, 384 } }, // LTC
    { 60, { 60, 0 } }, // ETH
    { 60001, { 0 } }, // Wrapped Ether WETH
    { 60002, { 0 } }, // USDT
    { 60003, { 0 } }, // USD coin
    { 9999, { 0 } }, // Ethereum Ropsten rETH
    { 9999001, { 0 } }, // Wrapped Ether Ropsten rETH
    { 88, { 0 } }, // Ethereum Rinkeby
    { 88001, { 0 } }, // Wrapped Ether Ropsten rETH
    { 384, { 0, 2, 384 } } }; // XSN

const unsigned MAX_PREFIX = 145;
constexpr qint64 COIN = 100000000;

class Enums {
    Q_GADGET
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")

public:
    enum class AddressType { P2WPKH, P2WSH, P2PKH, NONE };
    Q_ENUM(AddressType)

    enum class ValidationState { Validated, Failed, None };
    Q_ENUM(ValidationState)

    enum class OrderType { Limit, Market };
    Q_ENUM(OrderType)

    enum class OrderSide { Buy, Sell };
    Q_ENUM(OrderSide)

    enum class NotificationType { Withdraw, Swap, Deposit, BuyOrderPlaced, SellOrderPlaced };
    Q_ENUM(NotificationType)

    enum TransactionFlags : uint32_t {
        NoFlags = 0,
        PaymentToMyself = 1 << 0,
        Conflicted = 1 << 1,
        Abandoned = 1 << 2,
    };

    Q_FLAG(TransactionFlags)

    enum class DexTradingStatus { Online, Offline, Syncing };
    Q_ENUM(DexTradingStatus)

    enum class PaymentNodeType { Lnd, Connext };
    Q_ENUM(PaymentNodeType)

    enum class GasType { Slow, Average, Fast };
    Q_ENUM(GasType)
};
Q_DECLARE_METATYPE(Enums::AddressType)
Q_DECLARE_METATYPE(Enums::ValidationState)
Q_DECLARE_METATYPE(Enums::OrderSide)
Q_DECLARE_METATYPE(Enums::PaymentNodeType)

using AddressTypes = std::vector<Enums::AddressType>;

/*!
 * \brief RegisterQtTypes call once to register types in qt meta type system.
 */
void RegisterCommonQtTypes();

QVector<Enums::AddressType> GetSupportedAddressTypes(const CoinAsset& asset);
QString FormatAmount(Balance amount, unsigned numberOfDecimals = 8);
qint64 ParseUserInputCoin(QString value);
qint64 ParseAmount(const QJsonValue& value);
QDir GetDataDir(bool emulated);
QDir GetCrashReportingDir(bool emulated);
QDir GetLndDir(bool emulated);
void CleanLndDir(bool emulated);
QString GetChainIndexDir(bool emulated);
std::string AddressTypeToString(Enums::AddressType addressType);
Enums::AddressType AddressTypeStrToEnum(std::string addressType);
std::array<AddressEntry, MAX_PREFIX> GetAddressDetails();
size_t GetMaxReorgDepth(bool emulated);

void ResetWallet(bool isEmulated);

QString GetTxDBPath(bool emulated);
QString GetSwapRepositoryPath(bool emulated);
std::string GetPathForWalletDb(bool emulated);
QString DateDifference(QDateTime dateFrom, QDateTime dateTo, bool includeLessThanMinute = false,
    bool fullWord = false, bool showFullTime = false);
Balance ConvertToSatsPerByte(Balance satsPerKByte);
Balance ConvertToSatsPerKByte(double satsPerByte);
Balance ConvertFromWeiToSats(QString balance);
std::string ConvertFromDecimalWeiToHex(double value);

#endif // COMMON_HPP
