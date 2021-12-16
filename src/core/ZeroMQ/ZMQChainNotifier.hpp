#ifndef ZMQCHAINNOTIFIER_HPP
#define ZMQCHAINNOTIFIER_HPP

#include <Chain/BlockHeader.hpp>
#include <QObject>
#include <ZeroMQ/ZMQAbstractNotifier.hpp>

class AbstractChainDataSource;

class ZMQChainNotifier : public ZMQAbstractNotifier {
    Q_OBJECT
public:
    explicit ZMQChainNotifier(AbstractChainDataSource& chainDataSource, QObject* parent = nullptr);
    ~ZMQChainNotifier() override;

    bool Initialize() override;
    void Shutdown() override;

    /* send zmq multipart message
       parts:
          * command
          * data
          * message sequence number
    */
    bool sendMessage(const char* command, const void* data, size_t size);

    bool rescan(QString startBlockHash, std::pair<QString, size_t> endBlock);
    void abortRescan();

    bool notifyBlockHeader(const Wire::VerboseBlockHeader& header);
    bool notifyTip(const BlockHash& blockHash);

private:
    //    void onBlockHeadersReceived(AssetID assetID, std::vector<Wire::VerboseBlockHeader>
    //    headers, std::pair<QString, size_t> endBlock);

private:
    uint32_t nSequence{ 0U }; //!< upcounting per message sequence number
    bool _rescanInProgress{ false };
    AbstractChainDataSource& _chainDataSource;
};

#endif // ZMQCHAINNOTIFIER_HPP
