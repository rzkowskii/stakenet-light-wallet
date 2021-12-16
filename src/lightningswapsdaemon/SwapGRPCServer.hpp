#ifndef SWAPSGRPCSERVER_HPP
#define SWAPSGRPCSERVER_HPP

#include <GRPCTools/ServerUtils.hpp>
#include <Protos/lssdrpc.grpc.pb.h>
#include <SwapService.hpp>
#include <SwapsServiceNotifications.hpp>
#include <stdint.h>
#include <string>

class LssdSwapClientFactory;

class SwapGRPCServer : public qgrpc::BaseGrpcServer {
public:
    explicit SwapGRPCServer(
        swaps::SwapService& swapService, LssdSwapClientFactory& swapClientFactory, uint32_t port);

    void run();

protected:
    void registerService() override;

private:
    swaps::SwapService& _swapService;
    LssdSwapClientFactory& _swapClientFactory;
    SwapsServiceNotifications _swapsServiceNotifications;
    std::string _serverAddress;
    lssdrpc::tradingPairs::AsyncService _tradingPairsService;
    lssdrpc::orders::AsyncService _ordersService;
    lssdrpc::swaps::AsyncService _swapsService;
    lssdrpc::currencies::AsyncService _currenciesService;
    lssdrpc::renting::AsyncService _rentingService;
};

#endif // SWAPSGRPCSERVER_HPP
