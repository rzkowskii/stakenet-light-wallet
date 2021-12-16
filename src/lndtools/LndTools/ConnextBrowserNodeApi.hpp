#ifndef CONNEXTBROWSERNODEAPI_HPP
#define CONNEXTBROWSERNODEAPI_HPP

#include <LndTools/AbstractConnextApi.hpp>

//==============================================================================

/*!
 * \brief The ConnextBrowserNodeApiTransport class implements a way to interract with browser node
 * which is running inside of WebEngine which is part of QML framework. We basically need
 * it to communicate with qml web channel which talks to the browser node which is runing inside of WebView.
 * The only reason is that we have WebView <- WebChannel <- ConnextBrowserNodeApiTransport is that WebChannel cannot be created
 * from cpp because of attached properites are available only in private headers.
 * We are using model which is based on sequence number to map requests to responses.
 */
class ConnextBrowserNodeApiTransport : public QObject {
    Q_OBJECT
public:
    explicit ConnextBrowserNodeApiTransport(QObject* parent = nullptr);

    template <typename Func>
    typename std::enable_if<QtPrivate::FunctionPointer<Func>::IsPointerToMemberFunction
            && !std::is_convertible<Func, const char*>::value,
        bool>::type
    invoke(Func function, QVariant payload, QtPromise::QPromiseResolve<QVariant> resolve,
        QtPromise::QPromiseReject<QVariant> reject)
    {
        auto seq = generateSeq();
        _handlers.emplace(seq, ResolveReject{ resolve, reject });
        std::bind(function, this, seq, payload)();
        return true;
    }

signals:
    void initialize(qint32 seq, QVariant request);
    void setup(qint32 seq, QVariant channelInfo);
    void getConfig(qint32 seq, QVariant dummy);
    void getStateChannels(qint32 seq, QVariant dummy);
    void getStateChannel(qint32 seq, QVariant request);
    void conditionalTransfer(qint32 seq, QVariant request);
    void resolveTransfer(qint32 seq, QVariant request);
    void reconcileDeposit(qint32 seq, QVariant request);
    void withdrawDeposit(qint32 seq, QVariant request);
    void sendDepositTx(qint32 seq, QVariant request);
    void restoreState(qint32 seq, QVariant request);
    void eventConditionalTransferCreated(QVariant payload);
    void eventConditionalTransferResolved(QVariant payload);
    void getTransfers(qint32 seq, QVariant payload);

public slots:
    void dispatch(qint32 seq, QVariant payload);
    void dispatchError(qint32 seq, QVariant error);

private:
    qint32 generateSeq() const;

private:
    using ResolveReject
        = std::tuple<QtPromise::QPromiseResolve<QVariant>, QtPromise::QPromiseReject<QVariant>>;
    std::map<qint32, ResolveReject> _handlers;
};

//==============================================================================

/*!
 * \brief The ConnextBrowserNodeApi class implements connext API backed by browser node, effectively uses ConnextBrowserNodeApiTransport
 * to do all calls to web engine.
 */
class ConnextBrowserNodeApi : public AbstractConnextApi {
    Q_OBJECT
public:
    using SendTransactionDelegate = std::function<Promise<void>(DepositTxParams)>;
    explicit ConnextBrowserNodeApi(
        SendTransactionDelegate sendTxDelegate, QObject* parent = nullptr);

    Promise<QString> getPublicIdentifier() override;
    Promise<std::vector<QString>> getChannelsAddresses(QString publicIdentifier) override;
    Promise<QVariantMap> getChannel(QString publicIdentifier, QString channelAddress) override;
    Promise<QByteArray> transferResolve(QVariantMap payload) override;
    Promise<QVector<QVariantMap>> getChannelsList(QString publicIdentifier) override;
    Promise<QString> transferCreate(QVariantMap payload) override;
    Promise<QString> setupChannel(QVariantMap payload) override;
    Promise<void> sendDeposit(DepositTxParams payload) override;
    Promise<QString> reconcile(QVariantMap payload) override;
    Promise<void> withdraw(QVariantMap payload) override;
    Promise<void> initNode(QString secret) override;
    Promise<QString> restoreState(QVariantMap payload) override;
    Promise<QVector<QVariantMap>> getTransfers(QVariantMap payload) override;

    bool isActive() override;

    ConnextBrowserNodeApiTransport* transport() const;

private:
    ConnextBrowserNodeApiTransport* _transport{ nullptr };
    SendTransactionDelegate _sendTxDelegate;
    QString _publicIdentifier;
};

//==============================================================================

#endif // CONNEXTBROWSERNODEAPI_HPP
