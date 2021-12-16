#ifndef COINASSET_HPP
#define COINASSET_HPP

#include <QString>
#include <Tools/Common.hpp>
#include <chainparams.hpp>

class QJsonObject;

//==============================================================================

struct AssetMisc {
    AssetMisc(QString color, QString explorerLink, QString officialLink, QString redditLink,
        QString twitterLink, QString telegramLink, QString coinDescription, bool isAlwaysActive,
        QString defaultAddressType, unsigned confirmationsForApproved, QString rescanStartHash,
        double minLndCapacity, double maxLndCapacity, unsigned averageSycBlockForSec,
        double minPaymentAmount, double maxPaymentAmount, unsigned averageTxSize);
    static AssetMisc FromJson(const QJsonObject& obj);

    QString color;
    QString explorerLink;
    QString officialLink;
    QString redditLink;
    QString twitterLink;
    QString telegramLink;
    QString coinDescription;
    bool isAlwaysActive;
    Enums::AddressType defaultAddressType;
    unsigned confirmationsForApproved;
    QString rescanStartHash;
    double minLndCapacity;
    double maxLndCapacity;
    unsigned averageSycBlockForSec;
    double minPaymentAmount;
    double maxPaymentAmount;
    unsigned averageTxSize;
};

//==============================================================================

struct AssetLndData {
    static AssetLndData FromJson(const QJsonObject& obj);

    bool supportsLnd{ false };
    double minChannelCapacity;
    double maxChannelCapacity;
    double minPaymentAmount;
    double maxPaymentAmount;
    unsigned defaultSatsPerByte;
    unsigned blocksForEstimateFee;
    unsigned confirmationForChannelApproved;
    unsigned rentalChannelsPerCoin;
    double autopilotDefaultAllocation;
    unsigned autopilotDefaultMaxChannels;

private:
    AssetLndData() = default;
};

//==============================================================================

struct AssetConnextData {
    static AssetConnextData FromJson(const QJsonObject& obj);

    QString tokenAddress;

private:
    AssetConnextData() = default;
};

//==============================================================================

/*!
 * \brief The Token struct holds token related info
 */
struct Token {
    Token(AssetID chainAssetID, QString contract, uint32_t decimals);

    static Token FromJson(AssetID chainAssetID, const QJsonObject& obj);

    AssetID chainID() const;
    QString contract() const;
    uint32_t decimals() const;

private:
    AssetID _chainAssetID;
    QString _contract;
    uint32_t _decimals{ 18 };
};

//==============================================================================

struct ChainParams {
    ChainParams(bitcoin::CChainParams params, std::optional<uint32_t> chainId);

    bitcoin::CChainParams params;
    std::optional<uint32_t> chainId;
};

//==============================================================================

struct CoinAsset {
    CoinAsset(AssetID coinID, QString name, QString ticket, ChainParams params, AssetMisc misc,
        AssetLndData lndData, AssetConnextData connextData, std::optional<Token> token);

    enum class Type { UTXO, Account, Invalid };

    static CoinAsset FromJson(const QJsonObject& obj);
    static CoinAsset TokenFromJson(const CoinAsset& chainAsset, const QJsonObject& obj);

    AssetID coinID() const;
    Type type() const;

    void setName(QString name);
    QString name() const;

    void setTicket(QString ticket);
    QString ticket() const;

    void setParams(ChainParams params);
    const ChainParams& params() const;

    void setMisc(AssetMisc misc);
    const AssetMisc& misc() const;

    void setLndData(AssetLndData lndData);
    const AssetLndData& lndData() const;

    void setConnextData(AssetConnextData connextData);
    const AssetConnextData& connextData() const;

    /*!
     * \brief token returns token info about this asset, if this is a token on some chain
     * you can receive info about token using this structure. If it's not a token,
     * returns nullopt_t
     * \return Token in case asset is a token, nullopt_t otherwise
     */
    std::optional<Token> token() const;

private:
    AssetID _coinID;
    Type _assetType{ Type::Invalid };
    QString _name;
    QString _ticket;
    ChainParams _params;
    AssetMisc _misc;
    AssetLndData _lndData;
    AssetConnextData _connextData;

    std::optional<Token> _token;
};

//==============================================================================

#endif // COINASSET_HPP
