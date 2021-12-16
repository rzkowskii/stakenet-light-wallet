#include "ZMQAbstractNotifier.hpp"
#include <assert.h>

const int ZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM;

//==============================================================================

ZMQAbstractNotifier::ZMQAbstractNotifier(QObject* parent)
    : QObject(parent)
    , psocket(nullptr)
    , outbound_message_high_water_mark(DEFAULT_ZMQ_SNDHWM)
{
}

//==============================================================================

ZMQAbstractNotifier::~ZMQAbstractNotifier()
{
    assert(!psocket);
}

//==============================================================================

std::string ZMQAbstractNotifier::GetType() const
{
    return type;
}

//==============================================================================

void ZMQAbstractNotifier::SetType(const std::string& t)
{
    type = t;
}

//==============================================================================

std::string ZMQAbstractNotifier::GetAddress() const
{
    return address;
}

//==============================================================================

void ZMQAbstractNotifier::SetAddress(const std::string& a)
{
    address = a;
}

//==============================================================================

int ZMQAbstractNotifier::GetOutboundMessageHighWaterMark() const
{
    return outbound_message_high_water_mark;
}

//==============================================================================

void ZMQAbstractNotifier::SetOutboundMessageHighWaterMark(const int sndhwm)
{
    if (sndhwm >= 0) {
        outbound_message_high_water_mark = sndhwm;
    }
}

//==============================================================================
