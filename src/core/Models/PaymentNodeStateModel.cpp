#include "PaymentNodeStateModel.hpp"

//==============================================================================

PaymentNodeStateModel::PaymentNodeStateModel(AssetID assetID, QObject* parent)
    : QObject(parent)
    , _assetID(assetID)
{
}

//==============================================================================

PaymentNodeStateModel::~PaymentNodeStateModel() {}

//==============================================================================

Enums::PaymentNodeType PaymentNodeStateModel::nodeType() const
{
    return _paymentNodeType;
}

//==============================================================================
