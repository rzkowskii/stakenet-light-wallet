#include "PaymentNodeInterface.hpp"

//==============================================================================

PaymentNodeInterface::PaymentNodeInterface(Enums::PaymentNodeType type, QObject* parent)
    : QObject(parent)
    , _type(type)
{
}

//==============================================================================

Enums::PaymentNodeType PaymentNodeInterface::type() const
{
    return _type;
}

//==============================================================================
