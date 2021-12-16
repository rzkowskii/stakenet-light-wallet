#include "CoinAsset.hpp"

#include <QJsonArray>
#include <QJsonObject>
#include <QStringList>

static const uint32_t TOKEN_PARENT_CHAIN_ID_OFFSET = 1000;

//==============================================================================

static ChainParams ChainParamsFromJson(QJsonObject obj)
{
    using namespace bitcoin;
    auto base58Prefixes = obj.value("base58Prefix").toObject();

    const std::map<std::string, CChainParams::Base58Type> mapping{
        { "pubkeyAddress", CChainParams::Base58Type::PUBKEY_ADDRESS },
        { "scriptAddress", CChainParams::Base58Type::SCRIPT_ADDRESS },
        { "secretKey", CChainParams::Base58Type::SECRET_KEY },
        { "extPublicKey", CChainParams::Base58Type::EXT_PUBLIC_KEY },
        { "extSecretKey", CChainParams::Base58Type::EXT_SECRET_KEY }
    };

    CChainParams::Base58TypesMap typesMap;
    for (auto&& key : base58Prefixes.keys()) {
        if (mapping.count(key.toStdString())) {
            std::vector<unsigned char> bytes;
            for (QJsonValueRef byte : base58Prefixes.value(key).toArray()) {
                bytes.emplace_back(byte.toInt());
            }
            typesMap.emplace(mapping.at(key.toStdString()), bytes);
        }
    }
    auto bech32HRP = obj.value("bech32_hrp").toString().toStdString();
    CChainParams btcParams(typesMap,
        static_cast<CChainParams::ExtCoinType>(obj.value("extCoinType").toInt()), bech32HRP);

    auto chainId = obj.contains("chainId")
        ? std::make_optional<uint32_t>(obj.value("chainId").toInt())
        : std::nullopt;

    return ChainParams(btcParams, chainId);
}

//==============================================================================

CoinAsset::CoinAsset(AssetID coinID, QString name, QString ticket, ChainParams params,
    AssetMisc data, AssetLndData lndData, AssetConnextData connextData, std::optional<Token> token)
    : _coinID(coinID)
    , _name(name)
    , _ticket(ticket)
    , _params(params)
    , _misc(data)
    , _lndData(lndData)
    , _connextData(connextData)
    , _token(token)
{
}

//==============================================================================

CoinAsset CoinAsset::FromJson(const QJsonObject& obj)
{
    auto chainParamsJson = obj.value("chainparams").toObject();
    CoinAsset asset(static_cast<AssetID>(obj.value("id").toInt()), obj.value("name").toString(),
        obj.value("symbol").toString(), ChainParamsFromJson(chainParamsJson),
        AssetMisc::FromJson(obj.value("misc").toObject()),
        AssetLndData::FromJson(obj.value("lnd").toObject()),
        AssetConnextData::FromJson(obj.value("connext").toObject()), std::nullopt);

    auto type = chainParamsJson.value("type").toString().toLower();
    if (type == "utxo") {
        asset._assetType = Type::UTXO;
    } else if (type == "account") {
        asset._assetType = Type::Account;
    }

    return asset;
}

//==============================================================================

CoinAsset CoinAsset::TokenFromJson(const CoinAsset& chainAsset, const QJsonObject& obj)
{
    auto token = Token::FromJson(chainAsset.coinID(), obj);
    auto tokenId = static_cast<AssetID>(obj.value("id").toInt());
    auto offsetTokenId = chainAsset.coinID() * TOKEN_PARENT_CHAIN_ID_OFFSET + tokenId;
    CoinAsset asset(offsetTokenId, obj.value("name").toString(), obj.value("symbol").toString(),
        chainAsset.params(), AssetMisc::FromJson(obj.value("misc").toObject()),
        chainAsset.lndData(), chainAsset.connextData(), { token });

    asset._assetType = Type::Account;

    return asset;
}

//==============================================================================

AssetID CoinAsset::coinID() const
{
    return _coinID;
}

//==============================================================================

CoinAsset::Type CoinAsset::type() const
{
    return _assetType;
}

//==============================================================================

void CoinAsset::setName(QString name)
{
    _name = name;
}

//==============================================================================

QString CoinAsset::name() const
{
    return _name;
}

//==============================================================================

void CoinAsset::setTicket(QString ticket)
{
    _ticket = ticket;
}

//==============================================================================

QString CoinAsset::ticket() const
{
    return _ticket;
}

//==============================================================================

void CoinAsset::setParams(ChainParams params)
{
    _params = params;
}

//==============================================================================

const ChainParams& CoinAsset::params() const
{
    return _params;
}

//==============================================================================

void CoinAsset::setMisc(AssetMisc misc)
{
    _misc = misc;
}

//==============================================================================

const AssetMisc& CoinAsset::misc() const
{
    return _misc;
}

//==============================================================================

void CoinAsset::setLndData(AssetLndData lndData)
{
    _lndData = lndData;
}

//==============================================================================

const AssetLndData& CoinAsset::lndData() const
{
    return _lndData;
}

//==============================================================================

const AssetConnextData& CoinAsset::connextData() const
{
    return _connextData;
}

//==============================================================================

void CoinAsset::setConnextData(AssetConnextData connextData)
{
    _connextData = connextData;
}

//==============================================================================

std::optional<Token> CoinAsset::token() const
{
    return _token;
}

//==============================================================================

AssetMisc::AssetMisc(QString color, QString explorerLink, QString officialLink, QString redditLink,
    QString twitterLink, QString telegramLink, QString coinDescription, bool isAlwaysActive,
    QString defaultAddressType, unsigned confirmationsForApproved, QString rescanStartHash,
    double minLndCapacity, double maxLndCapacity, unsigned averageSycBlockForSec,
    double minPaymentAmount, double maxPaymentAmount, unsigned averageTxSize)
    : color(color)
    , explorerLink(explorerLink)
    , officialLink(officialLink)
    , redditLink(redditLink)
    , twitterLink(twitterLink)
    , telegramLink(telegramLink)
    , coinDescription(coinDescription)
    , isAlwaysActive(isAlwaysActive)
    , defaultAddressType(AddressTypeStrToEnum(defaultAddressType.toStdString()))
    , confirmationsForApproved(confirmationsForApproved)
    , rescanStartHash(rescanStartHash)
    , minLndCapacity(minLndCapacity)
    , maxLndCapacity(maxLndCapacity)
    , averageSycBlockForSec(averageSycBlockForSec)
    , minPaymentAmount(minPaymentAmount)
    , maxPaymentAmount(maxPaymentAmount)
    , averageTxSize(averageTxSize)
{
}

//==============================================================================

AssetMisc AssetMisc::FromJson(const QJsonObject& obj)
{
    return AssetMisc(obj.value("color").toString(), obj.value("explorerLink").toString(),
        obj.value("officialLink").toString(), obj.value("redditLink").toString(),
        obj.value("twitterLink").toString(), obj.value("telegramLink").toString(),
        obj.value("coinDescription").toString(), obj.value("isAlwaysActive").toBool(),
        obj.value("defaultAddressType").toString(),
        static_cast<unsigned>(obj.value("confirmationsForApproved").toInt()),
        obj.value("rescanStartHash").toString(), obj.value("minLndCapacity").toDouble(),
        obj.value("maxLndCapacity").toDouble(),
        static_cast<unsigned>(obj.value("averageSycBlockForSec").toInt()),
        obj.value("minPaymentAmount").toDouble(), obj.value("maxPaymentAmount").toDouble(),
        static_cast<unsigned>(obj.value("averageTxSize").toInt()));
}

//==============================================================================

AssetLndData AssetLndData::FromJson(const QJsonObject& obj)
{
    AssetLndData result;
    result.supportsLnd = !obj.isEmpty();
    result.minChannelCapacity = obj.value("minChannelCapacity").toDouble();
    result.maxChannelCapacity = obj.value("maxChannelCapacity").toDouble();
    result.minPaymentAmount = obj.value("minPaymentAmount").toDouble();
    result.maxPaymentAmount = obj.value("maxPaymentAmount").toDouble();
    result.defaultSatsPerByte = static_cast<unsigned>(obj.value("defaultSatsPerByte").toInt());
    result.blocksForEstimateFee = static_cast<unsigned>(obj.value("blocksForEstimateFee").toInt());
    result.confirmationForChannelApproved
        = static_cast<unsigned>(obj.value("confirmationForChannelApproved").toInt());
    result.rentalChannelsPerCoin
        = static_cast<unsigned>(obj.value("rentalChannelsPerCoin").toInt());
    result.autopilotDefaultAllocation = obj.value("autopilotDefaultAllocation").toDouble();
    result.autopilotDefaultMaxChannels
        = static_cast<unsigned>(obj.value("autopilotDefaultMaxChannels").toInt());
    return result;
}

//==============================================================================

AssetConnextData AssetConnextData::FromJson(const QJsonObject& obj)
{
    AssetConnextData result;
    result.tokenAddress = obj.value("tokenAddress").toString();
    return result;
}

//==============================================================================

Token::Token(AssetID chainAssetID, QString contract, uint32_t decimals)
    : _chainAssetID(chainAssetID)
    , _contract(contract)
    , _decimals(decimals)
{
}

//==============================================================================

Token Token::FromJson(AssetID chainAssetID, const QJsonObject& obj)
{
    return Token(chainAssetID, obj.value("contract").toString(), obj.value("decimals").toInt(18));
}

//==============================================================================

AssetID Token::chainID() const
{
    return _chainAssetID;
}

//==============================================================================

QString Token::contract() const
{
    return _contract;
}

//==============================================================================

uint32_t Token::decimals() const
{
    return _decimals;
}

//==============================================================================

ChainParams::ChainParams(bitcoin::CChainParams params, std::optional<uint32_t> chainId)
    : params(params)
    , chainId(chainId)
{
}

//==============================================================================
