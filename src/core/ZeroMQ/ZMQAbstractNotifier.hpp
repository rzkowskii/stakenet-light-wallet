// Copyright (c) 2015-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_ZMQ_ZMQABSTRACTNOTIFIER_H
#define BITCOIN_ZMQ_ZMQABSTRACTNOTIFIER_H

#include <QObject>
#include <string>
#include <zmq.h>

class CBlockIndex;
class ZMQAbstractNotifier;

typedef ZMQAbstractNotifier* (*CZMQNotifierFactory)();

class ZMQAbstractNotifier : public QObject {
    Q_OBJECT
public:
    static const int DEFAULT_ZMQ_SNDHWM{ 1000 };

    explicit ZMQAbstractNotifier(QObject* parent = nullptr);
    virtual ~ZMQAbstractNotifier();

    template <typename T> static ZMQAbstractNotifier* Create(QObject* parent = nullptr)
    {
        return new T(parent);
    }

    std::string GetType() const;
    void SetType(const std::string& t);
    std::string GetAddress() const;
    void SetAddress(const std::string& a);
    int GetOutboundMessageHighWaterMark() const;
    void SetOutboundMessageHighWaterMark(const int sndhwm);

    virtual bool Initialize() = 0;
    virtual void Shutdown() = 0;

    void* psocket;

protected:
    std::string type;
    std::string address;
    int outbound_message_high_water_mark; // aka SNDHWM
};

#endif // BITCOIN_ZMQ_ZMQABSTRACTNOTIFIER_H
