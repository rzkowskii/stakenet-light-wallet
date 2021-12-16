#ifndef PAYMENTNODEINTERFACE_HPP
#define PAYMENTNODEINTERFACE_HPP

#include <QObject>
#include <Utils/Utils.hpp>
#include <Tools/Common.hpp>

class AbstractPaymentNodeProcessManager;

class PaymentNodeInterface : public QObject {
    Q_OBJECT
public:
    explicit PaymentNodeInterface(Enums::PaymentNodeType type, QObject* parent = nullptr);

    virtual Enums::PaymentNodeType type() const;
    virtual Promise<QString> identifier() const = 0;
    virtual Promise<void> closeAllChannels(unsigned feeSatsPerByte) = 0;
    virtual void refreshChannels() = 0;
    virtual Promise<bool> isChannelsOpened() const = 0;
    virtual AbstractPaymentNodeProcessManager* processManager() const = 0;

private:
    Enums::PaymentNodeType _type;
};

#endif // PAYMENTNODEINTERFACE_HPP
