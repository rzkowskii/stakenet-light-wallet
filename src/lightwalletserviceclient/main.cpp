#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDebug>
#include <iostream>
#include <memory>
#include <string>

#include <LndTools/Protos/LightWalletService.grpc.pb.h>
#include <golomb/gcs.h>
#include <grpcpp/grpcpp.h>
#include <streams.h>
#include <transaction.h>
#include <uint256.h>
#include <utilstrencodings.h>

using grpc::Channel;
using grpc::ClientContext;

using namespace std;

class LightWalletServiceClient {

public:
    LightWalletServiceClient(std::shared_ptr<Channel> channel)
        : stub_(lightwalletrpc::LightWalletService::NewStub(channel))
    {
    }

    void createContext(ClientContext& context) { context.AddMetadata("assetid", "384"); }

    void LoadSecondLayer(uint64_t startHeight)
    {
        lightwalletrpc::LoadCacheRequest request;
        request.set_startheight(startHeight);

        lightwalletrpc::LoadCacheResponse reply;
        ClientContext context;
        createContext(context);

        auto status = stub_->LoadSecondLayerCache(&context, request, &reply);

        if (status.ok()) {
            std::cout << "LoadSecondLayer succeeded." << std::endl;
        } else {
            std::cout << "LoadSecondLayer failed." << std::endl;
            std::cout << status.error_code() << ": " << status.error_message() << std::endl;
        }
    }

    std::vector<bitcoin::CMutableTransaction> GetBlockTransactions(std::string blockHash)
    {
        int64_t height = -1;
        {
            lightwalletrpc::BlockHash req;
            req.set_hash(blockHash);
            ClientContext context;
            createContext(context);
            lightwalletrpc::BlockHeader response;
            auto status = stub_->GetBlockHeaderVerbose(&context, req, &response);
            if (!status.ok())
                throw std::runtime_error(status.error_message());

            height = response.height();
        }

        std::vector<bitcoin::CMutableTransaction> result;

        int index = 2;
        while (true) {
            lightwalletrpc::GetRawTxByIndexRequest request;
            request.set_blocknum(height);
            request.set_txindex(index++);
            lightwalletrpc::GetRawTxByIndexResponse response;

            ClientContext context;
            createContext(context);
            auto status = stub_->GetRawTxByIndex(&context, request, &response);
            if (!status.ok())
                break;

            CDataStream ss(bitcoin::ParseHex(response.txhex()), bitcoin::SER_NETWORK,
                bitcoin::PROTOCOL_VERSION);
            bitcoin::CMutableTransaction tx;
            ss >> tx;
            result.emplace_back(tx);
        }

        return result;
    }

    void TestBlockFilter(QString blockHash)
    {
        std::vector<std::string> outpoints;
        lightwalletrpc::BlockHash request;
        request.set_hash(blockHash.toStdString());

        lightwalletrpc::FilterBlockResponse filterBlockResponse;

        for (auto&& tx : GetBlockTransactions(blockHash.toStdString())) {
            for (auto&& out : tx.vout) {
                outpoints.emplace_back(
                    std::string(out.scriptPubKey.begin(), out.scriptPubKey.end()));
            }
        }

        lightwalletrpc::BlockFilter responseFilter;
        {
            ClientContext context;
            createContext(context);
            auto status = stub_->GetBlockFilter(&context, request, &responseFilter);

            if (status.ok()) {
                bitcoin::Filter::key_t key;
                auto hashBlock = bitcoin::uint256S(blockHash.toStdString());
                std::memcpy(&key[0], hashBlock.begin(), key.size());
                std::vector<uint8_t> bytes = bitcoin::ParseHex(responseFilter.bytes());
                std::cout << "Filter: " << responseFilter.n() << " " << responseFilter.m() << " "
                          << responseFilter.p() << " " << responseFilter.bytes().size()
                          << std::endl;
                if (bytes.empty()) {
                    throw std::runtime_error("Invalid filter");
                }
                bitcoin::Filter blockFilter = bitcoin::Filter::FromNMPBytes(
                    key, responseFilter.n(), responseFilter.m(), responseFilter.p(), bytes);
                std::cout << "Matched: " << blockFilter.matchAny(outpoints)
                          << " outpoints:" << outpoints.size() << std::endl;
            } else {
                std::cout << status.error_code() << ": " << status.error_message() << std::endl;
            }
        }
    }

private:
    std::unique_ptr<lightwalletrpc::LightWalletService::Stub> stub_;
};

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    QString channel("localhost:12345");
    LightWalletServiceClient lightWalletClient(
        grpc::CreateChannel(channel.toStdString(), grpc::InsecureChannelCredentials()));

    try {
#if 0
        uint64_t startHeight = 100000;

        lightWalletClient.LoadSecondLayer(startHeight);
#endif
        lightWalletClient.TestBlockFilter(
            "7ee45df33ac4f3b37e7cfc9ec3a40856c8d636296531e78f7b863e653bbe8e93");

    } catch (std::exception& ex) {
        std::cout << "Failed: " << ex.what() << std::endl;
    }

    return 0;
}
