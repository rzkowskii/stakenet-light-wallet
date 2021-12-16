#ifndef CONNEXTHTTPRESOLVER_HPP
#define CONNEXTHTTPRESOLVER_HPP

#include <QObject>
#include <QThread>
#include <QtPromise>
#include <memory>

#include <Swaps/AbstractSwapConnextClient.hpp>
#include <Swaps/Types.hpp>
#include <boost/asio/io_context.hpp>

namespace swaps {

class ConnextHttpResolveService : public AbstractConnextResolveService {
    Q_OBJECT
public:
    explicit ConnextHttpResolveService(QObject* parent = nullptr);
    ~ConnextHttpResolveService();

    void start(std::string address, unsigned short port);
    void stop();

protected:
    void timerEvent(QTimerEvent* event) override;

private:
    struct HttpWorker;
    boost::asio::io_context _ioc{ 1 };
    std::unique_ptr<HttpWorker> _server;
    int _timerId{ -1 };
};
}

#endif // CONNEXTHTTPRESOLVER_HPP
