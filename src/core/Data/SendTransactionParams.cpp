#include "SendTransactionParams.hpp"

//==============================================================================

BaseSendTxParams::BaseSendTxParams(AssetID assetID, QString addressTo, double amount)
    : assetID(assetID)
    , addressTo(addressTo)
    , amount(amount)
{
}

//==============================================================================

bitcoin::UTXOSendTxParams::UTXOSendTxParams(AssetID assetID, QString addressTo, double amount,
    std::optional<int64_t> feeRate, bool substractFeeFromAmount)
    : BaseSendTxParams(assetID, addressTo, amount)
    , feeRate(feeRate)
    , substractFeeFromAmount(substractFeeFromAmount)
{
}

//==============================================================================

eth::AccountSendTxParams::AccountSendTxParams(AssetID assetID, QString addressTo, double amount,
    std::string nonce, std::string gasPrice, std::string gasLimit, std::string data, Enums::GasType gasType)
    : BaseSendTxParams(assetID, addressTo, amount)
    , nonce(nonce)
    , gasPrice(gasPrice)
    , gasLimit(gasLimit)
    , data(data)
    , gasType(gasType)
{
}

//==============================================================================
