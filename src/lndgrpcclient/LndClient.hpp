#ifndef LNDCLIENT_HPP
#define LNDCLIENT_HPP

#include <Utils/Utils.hpp>

#include <QObject>
#include <QtPromise>
#include <memory>

template <class T> using Promise = QtPromise::QPromise<T>;

class AbstractLndClient : public QObject {
    Q_OBJECT
public:
    explicit AbstractLndClient(QObject* parent = nullptr);

    virtual bool openConnection() = 0;
    virtual void subscribeForInvoices() = 0;

signals:
    void getInfoReady(QVariantMap info);
    void invoiceAdded(QString payreq);

public slots:
    virtual void getInfo() = 0;
    virtual void openChannel(QString pubKey, double amount) {}
};

class LndClient : public AbstractLndClient {
    Q_OBJECT
public:
    explicit LndClient(QString rpcChannel, std::string tlsCert, QObject* parent = nullptr);
    ~LndClient() override;

    bool openConnection() override;
    void subscribeForInvoices() override;

signals:
    void getInfoReady(QVariantMap info);
    void invoiceAdded(QString payreq);

public slots:
    void getInfo() override;

private:
    struct Impl;
    Utils::WorkerThread _worker;
    std::unique_ptr<Impl> _impl;
};

#endif // LNDCLIENT_HPP
