#include "ConnextHttpResolveService.hpp"
#include <Utils/Logging.hpp>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThreadPool>
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

//==============================================================================

namespace ip = boost::asio::ip; // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp; // from <boost/asio.hpp>
namespace http = boost::beast::http; // from <boost/beast/http.hpp>

//==============================================================================
namespace swaps {

struct ConnextHttpResolveService::HttpWorker {
public:
    HttpWorker(HttpWorker const&) = delete;
    HttpWorker& operator=(HttpWorker const&) = delete;

    HttpWorker(boost::asio::io_context& ioc, std::string address, unsigned short port,
        ConnextHttpResolveService* resolveService)
        : _acceptor(ioc, { boost::asio::ip::make_address(address), port })
        , _socket(ioc)
        , _resolveService(resolveService)
    {
    }

    void start()
    {
        accept();
        check_deadline();
    }

private:
    // The acceptor used to listen for incoming connections.
    boost::asio::basic_socket_acceptor<tcp, boost::asio::io_context::executor_type> _acceptor;

    // The socket for the currently connected client.
    boost::asio::basic_stream_socket<tcp, boost::asio::io_context::executor_type> _socket;

    // The buffer for performing reads
    boost::beast::flat_buffer _buffer{ 8192 };

    // The timer putting a time limit on requests.
    boost::asio::basic_waitable_timer<std::chrono::steady_clock> _requestDeadline{
        _acceptor.get_executor().context(), (std::chrono::steady_clock::time_point::max)()
    };

    // The request message.
    http::request<http::dynamic_body> _request;

    // The response message.
    http::response<http::dynamic_body> _response;

    ConnextHttpResolveService* _resolveService{ nullptr };

    void accept()
    {
        // Clean up any previous connection.
        boost::beast::error_code ec;
        _socket.close(ec);
        _buffer.consume(_buffer.size());
        _request = {};

        LogDebug() << "Accepting on socket";

        _acceptor.async_accept(_socket, [this](boost::beast::error_code ec) {
            if (ec) {
                LogCCritical(Connext) << "Failed to accept: " << ec.message().data();
                accept();
            } else {
                // Request must be fully processed within 60 seconds.
                _requestDeadline.expires_after(std::chrono::seconds(60));

                readRequest();
            }
        });
    }

    void readRequest()
    {
        LogDebug() << "Preparing async_read";
        http::async_read(
            _socket, _buffer, _request, [this](boost::beast::error_code ec, std::size_t size) {
                if (ec) {
                    LogCCritical(Swaps) << "Failed to async_read: " << ec.message().data();
                    accept();
                } else {
                    processRequest();
                }
            });
    }

    void processRequest()
    {
        LogDebug() << "Incomming request:" << _request.method_string().data()
                   << _request.target().data();
        switch (_request.method()) {
        case http::verb::post:
            if (_request.target() == "/created") {
                resolveHash(_request.body());
                break;
            } else if (_request.target() == "/resolved") {
                settleTransfer(_request.body());
                break;
            }

        default:
            // We return responses indicating an error if
            // we do not recognize the request method.
            sendBadResponse();
            break;
        }
    }

    void resolveHash(boost::beast::multi_buffer const& body)
    {
        auto bodyAsString = boost::beast::buffers_to_string(body.data());
        LogDebug() << "Got transfer created request:" << bodyAsString.data();
        //        https://connext.github.io/vector/reference/nodeAPI/#conditiona-transfer-created

        /*
        {
          "aliceIdentifier": "indra6RJffyPFY7D4pAkyooRxgXVyocxYkRUW3NFQVznHazn98vcadh",
          "bobIdentifier": "indra82HffkBSTeSuvuEApLqLvY4wStnQYk7ysxZfYR2MmkwEe4XBbt",
          "channelAddress": "0xaE55b9aDdcD1cF5F5ECbff958F8C610d58f896d9",
          "channelBalance": {
            "to": [
              "0x9caCb917abf1966C4AD59e1f7233D9C9A93568a2",
              "0xC2FD5FE40adb1cA722079a0314Bb8b166150BDDE"
            ],
            "amount": [
              "856",
              "509027"
            ]
          },
          "transfer": {
            "balance": {
              "to": [
                "0x9caCb917abf1966C4AD59e1f7233D9C9A93568a2",
                "0xC2FD5FE40adb1cA722079a0314Bb8b166150BDDE"
              ],
              "amount": [
                "127",
                "0"
              ]
            },
            "assetId": "0x0000000000000000000000000000000000000000",
            "transferId": "0x07617f96952ca8f53d89387cb13272251bbef0e3a6d2623ddec0a349195ca7e5",
            "channelAddress": "0xaE55b9aDdcD1cF5F5ECbff958F8C610d58f896d9",
            "transferDefinition": "0xd69ABEf7eF08957F47d5B060B63D0b71Ee8d4621",
            "transferEncodings": [
              "tuple(bytes32 lockHash, uint256 expiry)",
              "tuple(bytes32 preImage)"
            ],
            "transferTimeout": "8640",
            "initialStateHash":
        "0xb40e01a1bde58598b8afe0c6e075eb44c28e70e234b10e4460c322154ade842a", "transferState": {
              "lockHash": "0x79a157d5572d9954c257c0ed29dc167b50579065acd8d329aa06e1344cf69f70",
              "expiry": "0"
            },
            "channelFactoryAddress": "0x70b3673264E9D56Cc8e53597bF3105eF281a5a0c",
            "chainId": 4,
            "initiator": "0x9caCb917abf1966C4AD59e1f7233D9C9A93568a2",
            "responder": "0xC2FD5FE40adb1cA722079a0314Bb8b166150BDDE",
            "meta": {
              "hello": "world123"
            }
          },
          "conditionType": "HashlockTransfer",
          "activeTransferIds": [
            "0x07617f96952ca8f53d89387cb13272251bbef0e3a6d2623ddec0a349195ca7e5"
          ]
        }

        */

        QJsonObject requestBody
            = QJsonDocument::fromJson(QByteArray::fromStdString(bodyAsString)).object();

        ResolveRequest request;
        request.rHash = requestBody.value("transfer")
                            .toObject()
                            .value("transferState")
                            .toObject()
                            .value("lockHash")
                            .toString()
                            .mid(2)
                            .toStdString();

        request.amount = static_cast<int64_t>((requestBody.value("transfer")
                                                   .toObject()
                                                   .value("balance")
                                                   .toObject()
                                                   .value("amount")
                                                   .toArray()
                                                   .first()
                                                   .toString()
                                                   .toDouble()));

        request.expiration
            = requestBody.value("transfer").toObject().value("transferTimeout").toString().toInt();
        request.initiatorIdentifier = requestBody.value("transfer")
                                          .toObject()
                                          .value("initiatorIdentifier")
                                          .toString()
                                          .toStdString();
        request.responderIdentifier = requestBody.value("transfer")
                                          .toObject()
                                          .value("responderIdentifier")
                                          .toString()
                                          .toStdString();
        request.transferId
            = requestBody.value("transfer").toObject().value("transferId").toString().toStdString();
        request.channelAddress = requestBody.value("transfer")
                                     .toObject()
                                     .value("channelAddress")
                                     .toString()
                                     .toStdString();

        QMetaObject::invokeMethod(_resolveService,
            std::bind(&ConnextHttpResolveService::receivedResolveRequest, _resolveService, request),
            Qt::QueuedConnection);

        auto response = QJsonDocument(QJsonObject{}).toJson(QJsonDocument::Compact);
        LogDebug() << "Sending response" << response;
        _response.set(http::field::content_type, "application/json");
        boost::beast::ostream(_response.body()) << response.toStdString();
        this->writeResponse();
    }

    void settleTransfer(boost::beast::multi_buffer const& body)
    {
        auto bodyAsString = boost::beast::buffers_to_string(body.data());
        LogDebug() << "Got transfer resolved request:" << bodyAsString.data();

        QJsonObject requestBody
            = QJsonDocument::fromJson(QByteArray::fromStdString(bodyAsString)).object();

        ResolvedTransferResponse request;
        request.lockHash = requestBody.value("transfer")
                               .toObject()
                               .value("transferState")
                               .toObject()
                               .value("lockHash")
                               .toString()
                               .mid(2)
                               .toStdString();

        request.rPreimage = requestBody.value("transfer")
                                .toObject()
                                .value("transferResolver")
                                .toObject()
                                .value("preImage")
                                .toString()
                                .mid(2)
                                .toStdString();

        QMetaObject::invokeMethod(_resolveService,
            std::bind(
                &ConnextHttpResolveService::receivedResolvedTransfer, _resolveService, request));

        auto response = QJsonDocument(QJsonObject{}).toJson(QJsonDocument::Compact);
        LogDebug() << "Sending response" << response;
        _response.set(http::field::content_type, "application/json");
        boost::beast::ostream(_response.body()) << response.toStdString();
        this->writeResponse();
    }

    void writeResponse()
    {
        LogDebug() << "Writting response";
        _response.content_length(_response.body().size());
        http::async_write(_socket, _response, [this](boost::beast::error_code ec, std::size_t) {
            LogDebug() << "Response written:" << ec.message().data();
            _response = {};
            _socket.shutdown(tcp::socket::shutdown_send, ec);
            _requestDeadline.cancel();
            this->accept();
        });
    }

    void sendBadResponse()
    {
        _response.result(http::status::bad_request);
        _response.set(http::field::content_type, "text/plain");
        writeResponse();
    }

    void check_deadline()
    {
        // The deadline may have moved, so check it has really passed.
        if (_requestDeadline.expiry() <= std::chrono::steady_clock::now()) {
            LogDebug() << "Request expired, closing socket";
            // Close socket to cancel any outstanding operation.
            boost::beast::error_code ec;
            _socket.close();

            // Sleep indefinitely until we're given a new deadline.
            _requestDeadline.expires_at(std::chrono::steady_clock::time_point::max());
        }

        _requestDeadline.async_wait([this](boost::beast::error_code) { check_deadline(); });
    }
};

//==============================================================================

ConnextHttpResolveService::ConnextHttpResolveService(QObject* parent)
    : AbstractConnextResolveService(parent)
{
}

//==============================================================================

ConnextHttpResolveService::~ConnextHttpResolveService()
{
    stop();
}

//==============================================================================

void ConnextHttpResolveService::start(std::string address, unsigned short port)
{
    _timerId = startTimer(100);
    try {
        _server = std::make_unique<HttpWorker>(_ioc, address, port, this);
        LogDebug() << "Starting resolving on address " << QString::fromStdString(address) << ":"
                   << port;
        _server->start();
        _ioc.poll();
    } catch (std::exception& ex) {
        LogCCritical(Swaps) << "Failed to init connext http resolver:" << ex.what();
    }
}

//==============================================================================

void ConnextHttpResolveService::stop()
{
    if (_timerId >= 0) {
        killTimer(_timerId);
    }
}

//==============================================================================

void ConnextHttpResolveService::timerEvent(QTimerEvent*)
{
    _ioc.poll();
}

//==============================================================================
}
