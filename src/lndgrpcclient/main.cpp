#include <GRPCTools/ClientUtils.hpp>
#include <LndClient.hpp>
#include <LndTools/LndGrpcClient.hpp>
#include <QDebug>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <Tools/Common.hpp>
#include <utilstrencodings.h>

class LndMainImpl : public AbstractLndClient {

public:
    explicit LndMainImpl(
        QString rpcChannel, BaseGrpcClient::TlsCertProvider tlsCert, QObject* parent = nullptr)
        : AbstractLndClient(parent)
        , _grpcClient(rpcChannel, tlsCert)
    {
    }

    bool openConnection() override
    {
        _grpcClient.connect();
        return true;
    }
    void subscribeForInvoices() override
    {
        auto context = _grpcClient.makeRpcStreamingRequest<lnrpc::Invoice>(
            &lnrpc::Lightning::Stub::PrepareAsyncSubscribeInvoices, lnrpc::InvoiceSubscription(),
            0);

        connect(context, &StreamingContext<lnrpc::Invoice>::readyRead, this, [=] {
            for (auto&& event : context->takeItems()) {
                qDebug() << "got event" << event.add_index() << "in" << this;
            }
        });
    }

public slots:
    void getInfo() override
    {
        _grpcClient
            .makeRpcUnaryRequest<lnrpc::GetInfoResponse>(
                &lnrpc::Lightning::Stub::PrepareAsyncGetInfo, lnrpc::GetInfoRequest())
            .then([](lnrpc::GetInfoResponse r) {
                qDebug() << "Got getinfo response:" << r.block_hash().c_str();
            });
    }

    void openChannel(QString pubKey, double amount) override
    {
        lnrpc::OpenChannelRequest req;
        pubKey = "0358dc7a0f1a296bc4310ba6dc2d2c208dab250cf73dc4616cb925a0fe40714714";
        auto bytes = bitcoin::ParseHex(pubKey.toStdString());
        req.set_node_pubkey(std::string(bytes.begin(), bytes.end()));
        req.set_local_funding_amount(COIN / 10);
        auto context = _grpcClient.makeRpcStreamingRequest<lnrpc::OpenStatusUpdate>(
            &lnrpc::Lightning::Stub::PrepareAsyncOpenChannel, req, 0);

        QtPromise::connect(context, &StreamingContext<lnrpc::OpenStatusUpdate>::readyRead,
            &StreamingContext<lnrpc::OpenStatusUpdate>::closed)
            .then([] { qDebug() << "Channel succesfully opened"; })
            .fail([](grpc::Status status) {
                qDebug() << "Failed with status" << status.error_message().c_str();
            })
            .fail([] { qDebug() << "Fail general"; })
            .finally([context] {
                qDebug() << "Finnaly";
                context->deleteLater();
            });
    }

private:
    LndGrpcClient _grpcClient;
};

int main(int argc, char* argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QGuiApplication app(argc, argv);

    QString rpcChannel = "localhost:10000";
    QString tlsCert = "/tmp/lnd/tls.cert";

    AbstractLndClient* client = nullptr;
#if 0
    LndClient clientImpl(rpcChannel, tlsCert);
#else
    LndMainImpl impl(rpcChannel, [tlsCert] { return Utils::ReadCert(tlsCert); });
    client = &impl;
#endif

    client->openConnection();
    client->subscribeForInvoices();
    client->subscribeForInvoices();

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("client", client);
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
