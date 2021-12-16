#include "LndGrpcClient.hpp"

#include <QTimer>

//==============================================================================

LndGrpcClient::LndGrpcClient(QString rpcChannel, TlsCertProvider tlsCertProvider,
    MacaroonProvider macaroonProvider, QObject* parent)
    : BaseGrpcClient(rpcChannel, tlsCertProvider,
          macaroonProvider ? macaroonProvider : [] { return std::string{}; },
          macaroonProvider ? AuthType::Macaroon : AuthType::SSL, parent)
{
    _connectionTimer = new QTimer(this);
    _connectionTimer->setInterval(2500);
    QObject::connect(_connectionTimer, &QTimer::timeout, this, &LndGrpcClient::tryConnect);
}

//==============================================================================

LndGrpcClient::~LndGrpcClient() {}

//==============================================================================

void LndGrpcClient::connect()
{
    close();
    _connectionTimer->start(_connectionTimer->interval());
    tryConnect();
}

//==============================================================================

bool LndGrpcClient::isConnected() const
{
    return _rpcClient != nullptr;
}

//==============================================================================

void LndGrpcClient::close()
{
    BaseGrpcClient::tearDown();
    _rpcClient.reset();
    _invoicesClient.reset();
    _autopilotClient.reset();
    _routerClient.reset();
    _watchTowerClient.reset();
}

//==============================================================================

void LndGrpcClient::tryConnect()
{
    try {
        if (BaseGrpcClient::init()) {
            BaseGrpcClient::connect();

            if (!_rpcClient) {
                _rpcClient = lnrpc::Lightning::NewStub(_channel);
                _invoicesClient = invoicesrpc::Invoices::NewStub(_channel);
                _autopilotClient = autopilotrpc::Autopilot::NewStub(_channel);
                _routerClient = routerrpc::Router::NewStub(_channel);
                _watchTowerClient = wtclientrpc::WatchtowerClient::NewStub(_channel);
            }

            _connectionTimer->stop();
            connected();
        }
    } catch (...) {
    }
}

//==============================================================================
