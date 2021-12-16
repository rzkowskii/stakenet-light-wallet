#ifndef SENDTRANSACTIONPARAMS_HPP
#define SENDTRANSACTIONPARAMS_HPP

#include <QString>
#include <variant>

#include <Tools/Common.hpp>

//==============================================================================

struct BaseSendTxParams {
    BaseSendTxParams(AssetID assetID, QString addressTo, double amount);

    AssetID assetID;
    QString addressTo; // address of receiver
    double amount{ 0.0 }; // amount in coins
};

//==============================================================================

namespace bitcoin {
struct UTXOSendTxParams : BaseSendTxParams {
    UTXOSendTxParams(AssetID assetID, QString addressTo, double amount,
        std::optional<int64_t> feeRate, bool substractFeeFromAmount);

    std::optional<int64_t> feeRate;
    bool substractFeeFromAmount{ true };
};
}

//==============================================================================

namespace eth {
struct AccountSendTxParams : BaseSendTxParams {
    AccountSendTxParams(AssetID assetID, QString addressTo, double amount, std::string nonce,
        std::string gasPrice, std::string gasLimit, std::string data, Enums::GasType gasType);
    std::string nonce; // hex nonce
    std::string gasPrice; // hex gas price
    std::string gasLimit; // hex gas limit
    std::string data; // hex data
    Enums::GasType gasType {Enums::GasType::Average};
};
}

//==============================================================================

using SendTransactionParams = std::variant<bitcoin::UTXOSendTxParams, eth::AccountSendTxParams>;

//==============================================================================

#endif // SENDTRANSACTIONPARAMS_HPP
