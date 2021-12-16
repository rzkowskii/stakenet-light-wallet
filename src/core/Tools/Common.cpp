#include "Common.hpp"
#include <Chain/BlockHeader.hpp>
#include <Data/CoinAsset.hpp>
#include <Data/TransactionEntry.hpp>
#include <EthCore/Encodings.hpp>
#include <EthCore/Types.hpp>
#include <Models/WalletMarketSwapModel.hpp>
#include <Networking/AbstractAccountExplorerHttpClient.hpp>
#include <Networking/AbstractBlockExplorerHttpClient.hpp>
#include <QDate>
#include <QJsonValue>
#include <QMetaType>
#include <QSettings>
#include <QStandardPaths>
#include <QtGui/QColor>
#include <Tools/AppConfig.hpp>
#include <Utils/Logging.hpp>
#include <cmath>
#include <inttypes.h>
#include <stdio.h>
#include <utilstrencodings.h>
#include <boost/multiprecision/cpp_dec_float.hpp>

//==============================================================================

static std::map<Enums::AddressType, std::string> addressTypes{ { Enums::AddressType::P2PKH,
                                                                   "p2pkh" },
    { Enums::AddressType::P2WPKH, "p2wpkh" }, { Enums::AddressType::P2WSH, "p2sh-p2wpkh" } };

//==============================================================================

void RegisterCommonQtTypes()
{
    static bool once = true;
    if (once) {
        qRegisterMetaType<Balance>("Balance");
        qRegisterMetaType<AssetID>("AssetID");
        qRegisterMetaType<OnChainTxRef>("OnChainTxRef");
        qRegisterMetaType<Transaction>("Transaction");
        qRegisterMetaType<std::vector<Transaction>>("std::vector<Transaction>");
        qRegisterMetaType<std::vector<OnChainTxRef>>("std::vector<OnChainTxRef>");
        qRegisterMetaType<MutableTransactionRef>("MutableTransactionPtr");
        qRegisterMetaType<std::vector<Wire::VerboseBlockHeader>>(
            "std::vector<Wire::VerboseBlockHeader>");
        qRegisterMetaType<Wire::StrippedBlock>("Wire::StrippedBlock");
        qRegisterMetaType<BlockHash>("BlockHash");
        qRegisterMetaType<BlockHeight>("BlockHeight");
        qRegisterMetaType<std::vector<AssetID>>("std::vector<AssetID>");
        qRegisterMetaType<QStringList>("QStringList");
        qRegisterMetaType<MarketSwapRequest>("MarketSwapRequest");
        qRegisterMetaType<SwapFee>("SwapFee");
        once = false;
    }
}

//==============================================================================

QVector<Enums::AddressType> GetSupportedAddressTypes(const CoinAsset& asset)
{
    if (!asset.params().params.bech32HRP().empty()) {
        return { Enums::AddressType::P2PKH, Enums::AddressType::P2WPKH, Enums::AddressType::P2WSH };
    } else {
        return { Enums::AddressType::P2PKH };
    }
}

//==============================================================================

QString FormatAmount(Balance amount, unsigned numberOfDecimals)
{
    if (amount == 0)
        return QString("0.0");

    qint64 n = std::abs(amount);
    qint64 n_abs = (n > 0 ? n : -n);
    qint64 quotient = n_abs / COIN;
    QString quotient_str = QString::number(quotient);

    if (numberOfDecimals > 0) {
        qint64 remainder = n_abs % COIN;
        QString remainder_str
            = QString::number(remainder).rightJustified(8, '0', true).left(numberOfDecimals);
        return quotient_str + QString(".") + remainder_str;
    } else {
        return quotient_str;
    }
}

//==============================================================================

qint64 ParseAmount(const QJsonValue& value)
{
    int64_t amount;
    if (!bitcoin::ParseFixedPoint(
            QString::number(value.toDouble(), 'f', 8).toStdString(), 8, &amount))
        throw std::runtime_error("Failed to parse json value");

    return amount;
}

//==============================================================================

QDir GetDataDir(bool emulated)
{
#ifdef Q_OS_ANDROID
    QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
#else
    QDir dir(QStandardPaths::writableLocation(
        emulated ? QStandardPaths::TempLocation : QStandardPaths::GenericDataLocation));
#endif

    auto innerPath = "Stakenet/stakenet-wallet";

    if (!emulated) {
        auto dataDir = AppConfig::Instance().config().dataDirPath;
        if (!dataDir.isEmpty()) {
            dir.setPath(dataDir);
            innerPath = ".";
        }
    }

    if (!dir.exists(innerPath)) {
        dir.mkpath(innerPath);
    }

    dir.cd(innerPath);

    return dir;
}

//==============================================================================

void CleanLndDir(bool emulated)
{
    GetLndDir(emulated).removeRecursively();
}

//==============================================================================

QDir GetLndDir(bool emulated)
{
    auto lndDir = GetDataDir(emulated);
    auto innerPath = QString("lnd/swap");

    if (!lndDir.exists(innerPath)) {
        lndDir.mkpath(innerPath);
    }

    lndDir.cd(innerPath);

    return lndDir;
}

//==============================================================================

QString GetChainIndexDir(bool emulated)
{
    const auto& folderName = "chain/index";
    QDir path = GetDataDir(emulated);
    if (!path.exists(folderName)) {
        path.mkpath(folderName);
    }

    path.cd(folderName);
    return path.absolutePath();
}

//==============================================================================

std::string AddressTypeToString(Enums::AddressType addressType)
{
    return addressTypes.count(addressType) > 0 ? addressTypes.at(addressType) : std::string();
}

//==============================================================================

std::array<AddressEntry, MAX_PREFIX> GetAddressDetails()
{
    std::array<AddressEntry, MAX_PREFIX> result;

    std::vector<QString> keys = { "2", "3", "4", "5", "6", "7", "8", "9", "A", "B", "C", "D", "E",
        "F", "G", "H", "J", "K", "L", "M", "N", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y",
        "Z", "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "m", "n", "o", "p", "q", "r",
        "s", "t", "u", "v", "w", "x", "y", "z" };

    for (size_t key = 0, pos = 3; pos <= 81; key += 2, pos += 5) {
        QString prefix1 = keys.at(key);
        QString prefix2 = keys.at(key + 1);
        QString prefix3 = keys.at(key + 2);

        result[pos] = AddressEntry({ prefix1 }, 34);
        result[pos + 1] = AddressEntry({ prefix1, prefix2 }, 34);
        result[pos + 2] = AddressEntry({ prefix2 }, 34);
        result[pos + 3] = result[pos + 2];
        result[pos + 4] = AddressEntry({ prefix2, prefix3 }, 34);
    }

    for (size_t key = 32, pos = 83; pos <= 142; key += 2, pos += 5) {
        QString prefix1 = keys.at(key);
        QString prefix2 = keys.at(key + 1);
        QString prefix3 = keys.at(key + 2);

        result[pos] = AddressEntry({ prefix1 }, 34);
        result[pos + 1] = AddressEntry({ prefix1, prefix2 }, 34);
        result[pos + 2] = AddressEntry({ prefix2 }, 34);
        result[pos + 3] = AddressEntry({ prefix2, prefix3 }, 34);
        result[pos + 4] = AddressEntry({ prefix3 }, 34);
    }

    result[0] = AddressEntry({ "1" }, 34);

    result[1] = AddressEntry({ "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "a", "b", "c", "d",
                                 "e", "f", "g", "h", "i", "j", "k", "m", "n", "o" },
        33);
    result[2] = AddressEntry({ "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z" }, 34);
    result[143] = AddressEntry({ "z" }, 34);
    result[144] = AddressEntry({ "z", "2" }, 35);

    return result;
}

//==============================================================================

Enums::AddressType AddressTypeStrToEnum(std::string addressType)
{

    addressType = QString::fromStdString(addressType).toLower().toStdString();
    auto it = std::find_if(std::begin(addressTypes), std::end(addressTypes),
        [addressType](const auto& it) { return it.second == addressType; });

    return it != std::end(addressTypes) ? it->first : Enums::AddressType::NONE;
}

//==============================================================================

QString GetTxDBPath(bool emulated)
{
    const auto& folderName = "chain/transactions";
    QDir path = GetDataDir(emulated);
    if (!path.exists(folderName)) {
        path.mkpath(folderName);
    }
    return path.absoluteFilePath(folderName);
}

//==============================================================================

std::string GetPathForWalletDb(bool emulated)
{
    QDir path = GetDataDir(emulated);
    const auto& folderName = "wallets";
    if (!path.exists(folderName)) {
        path.mkpath(folderName);
    }

    path.cd(folderName);
    return path.absolutePath().toStdString();
}

//==============================================================================

size_t GetMaxReorgDepth(bool emulated)
{
    return emulated ? 10000 : 1000;
}

//==============================================================================

QString DateDifference(QDateTime dateFrom, QDateTime dateTo, bool includeLessThanMinute,
    bool fullWord, bool showFullTime)
{
    if (dateTo.toSecsSinceEpoch() < dateFrom.toSecsSinceEpoch()) {
        // qDebug() << "Cannot calculate difference, dateTo is less than dateFrom";
        return QString("Expired");
    }
    auto from = dateFrom.date();
    auto to = dateTo.date();

    int seconds = 0;
    int minutes = 0;
    int hours = 0;
    int days = 0;
    int months = 0;
    int years = 0;

    int monthDay[12] = { 31, -1, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    auto increment = 0;

    // days calculation
    if (from.day() > to.day()) {
        increment = monthDay[from.month() - 1];
        if (increment == -1) {
            if (QDate::isLeapYear(from.year())) {
                increment = 29;
            } else {
                increment = 28;
            }
        }
    }

    if (increment != 0) {
        days = (to.day() + increment) - from.day();
        increment = 1;
    } else {
        days = to.day() - from.day();
    }

    // hours calculation
    auto hourDiff = dateTo.time().hour() - dateFrom.time().hour();
    auto minuteDiff = dateTo.time().minute() - dateFrom.time().minute();

    if (hourDiff >= 0) {
        hours = hourDiff;
    } else {
        hours = 24 - std::abs(hourDiff);
        days--;
    }

    auto secondsDiff = dateTo.time().second() - dateFrom.time().second();
    if (minuteDiff >= 0) {
        minutes = minuteDiff;
    } else {
        minutes = 60 - std::abs(minuteDiff);
        hours--;
    }

    if (secondsDiff < 0) {
        minutes--;
    }

    // mounth calculation
    if ((from.month() + increment) > to.month()) {
        months = (to.month() + 12) - (from.month() + increment);
        increment = 1;
    } else {
        months = (to.month()) - (from.month() + increment);
        increment = 0;
    }
    // year calculation
    years = to.year() - (from.year() + increment);
    auto onlySeconds = minutes <= 0 && hours <= 0 && days <= 0 && months <= 0 && years <= 0;

    auto yearsSymbol = years > 1 ? "years" : "year";
    auto monthsSymbol = months > 1 ? "months" : "month";
    auto daysSymbol = days > 1 ? "days" : "day";
    auto hoursSymbol = hours > 1 ? "hours" : "hour";
    auto minutesSymbol = minutes > 1 ? "minutes" : "minute";

    auto hoursStr = QString("%1 %2").arg(hours).arg(hoursSymbol);
    auto minutesStr = QString("%1 %2").arg(minutes).arg(minutesSymbol);

    QString result;

    if (years > 0) {
        result.append(QString("%1 %2").arg(years).arg(fullWord ? yearsSymbol : "Y"));
    }
    if (months > 0) {
        if (!result.isEmpty()) {
            result.append(" ");
        }
        result.append(QString("%1 %2").arg(months).arg(fullWord ? monthsSymbol : "M"));
    }
    if (days > 0) {
        if (!result.isEmpty()) {
            result.append(" ");
        }
        result.append(QString("%1 %2").arg(days).arg(fullWord ? daysSymbol : "D"));
    }
    if (hours > 0) {
        auto resultHours = fullWord ? (showFullTime ? hoursStr : (days <= 0 ? hoursStr : ""))
                                    : QString("%1%2").arg(hours).arg("H");
        if (!result.isEmpty() && !resultHours.isEmpty()) {
            result.append(" ");
        }
        result.append(resultHours);
    }
    if (minutes > 0) {
        auto resultMinutes = fullWord
            ? (showFullTime ? minutesStr : (days <= 0 && hours <= 0 ? minutesStr : ""))
            : QString("%1%2").arg(minutes).arg("m");
        if (!result.isEmpty() && !resultMinutes.isEmpty()) {
            result.append(" ");
        }
        result.append(resultMinutes);
    }
    if (includeLessThanMinute) {
        auto resultSeconds = seconds == 0 && onlySeconds
            ? "just now"
            : (seconds == 0 ? "" : QString("%1s").arg(seconds));
        if (!result.isEmpty() && !resultSeconds.isEmpty()) {
            result.append(" ");
        }
        result.append(resultSeconds);
    } else {
        QString resultSeconds = seconds == 0 && onlySeconds ? "less than a minute" : "";
        if (!result.isEmpty() && !resultSeconds.isEmpty()) {
            result.append(" ");
        }
        result.append(resultSeconds);
    }
    return result;
}

//==============================================================================

QDir GetCrashReportingDir(bool emulated)
{
    auto rootDataDir = GetDataDir(emulated);
    auto path = "reports/crash";
    if (!rootDataDir.exists(path)) {
        rootDataDir.mkpath(path);
    }

    return rootDataDir.absoluteFilePath(path);
}

//==============================================================================

qint64 ParseUserInputCoin(QString value)
{
    if (value.isEmpty()) {
        return -1;
    }

    while (value.back() == '0' && value.contains(".")) {
        value = value.left(value.size() - 1);
    }

    auto dotIndex = value.indexOf(".");

    auto remainder = 0;
    if ((dotIndex != -1)) {
        bool isOk;
        auto remainderStr = value.mid(dotIndex + 1);
        auto tmp = remainderStr.toInt(&isOk);
        if (isOk && tmp > 0) {
            remainder = tmp * (COIN / qint64(std::pow(10, remainderStr.size())));
        }
    }

    auto quotient = value.left(dotIndex).toInt();
    qint64 result = (quotient * COIN) + remainder;
    return result;
}

//==============================================================================

QString GetSwapRepositoryPath(bool emulated)
{
    const auto& folderName = "swaps";
    QDir path = GetDataDir(emulated);
    if (!path.exists(folderName)) {
        path.mkpath(folderName);
    }
    return path.absoluteFilePath(folderName);
}

//==============================================================================

void ResetWallet(bool isEmulated)
{
    QDir txDB(GetTxDBPath(isEmulated));
    QDir walletDir(QString::fromStdString(GetPathForWalletDb(isEmulated)));
    if (txDB.exists() && !txDB.removeRecursively()) {
        LogCCritical(General) << "Failed to remove txdb";
    }

    if (walletDir.exists() && !walletDir.removeRecursively()) {
        LogCCritical(General) << "Failed to remove wallet";
    }
}

//==============================================================================

Balance ConvertToSatsPerByte(Balance satsPerKByte)
{
    return static_cast<qint64>(satsPerKByte / 1000);
}

//==============================================================================

Balance ConvertToSatsPerKByte(double satsPerByte)
{
    return static_cast<qint64>(satsPerByte * 1000);
}

//==============================================================================

Balance ConvertFromWeiToSats(QString balance)
{
    auto ethBalance = eth::u256{ balance.toStdString() };
    ethBalance /= eth::exp10<10>();
    return ethBalance.convert_to<int64_t>();
}

//==============================================================================

std::string ConvertFromDecimalWeiToHex(double value)
{
    boost::multiprecision::cpp_dec_float_50 wei(value);
    std::stringstream ss;
    ss << std::hex << std::showbase << static_cast<eth::u256>(wei);
    return ss.str();
}

//==============================================================================
