#include "ZMQChainNotifier.hpp"
#include <Chain/AbstractChainDataSource.hpp>
#include <Tools/DBUtils.hpp>
#include <Utils/Logging.hpp>
#include <crypto/common.h>
#include <serialize.h>
#include <stdarg.h>
#include <streams.h>

//==============================================================================

static std::multimap<std::string, ZMQAbstractNotifier*> mapPublishNotifiers;

static const char* MSG_BLOCKHEADER = "rawheader";
static const char* MSG_HASHBLOCK = "hashblock";

//==============================================================================

// Internal function to send multipart message
static int zmq_send_multipart(void* sock, const void* data, size_t size, ...)
{
    va_list args;
    va_start(args, size);

    while (1) {
        zmq_msg_t msg;

        int rc = zmq_msg_init_size(&msg, size);
        if (rc != 0) {
            //            zmqError("Unable to initialize ZMQ msg");
            va_end(args);
            return -1;
        }

        void* buf = zmq_msg_data(&msg);
        memcpy(buf, data, size);

        data = va_arg(args, const void*);

        rc = zmq_msg_send(&msg, sock, data ? ZMQ_SNDMORE : 0);
        if (rc == -1) {
            //            zmqError("Unable to send ZMQ msg");
            zmq_msg_close(&msg);
            va_end(args);
            return -1;
        }

        zmq_msg_close(&msg);

        if (!data)
            break;

        size = va_arg(args, size_t);
    }
    va_end(args);
    return 0;
}

//==============================================================================

ZMQChainNotifier::ZMQChainNotifier(AbstractChainDataSource& chainDataSource, QObject* parent)
    : ZMQAbstractNotifier(parent)
    , _chainDataSource(chainDataSource)
{
}

//==============================================================================

ZMQChainNotifier::~ZMQChainNotifier()
{
    if (psocket) {
        Shutdown();
    }
}

//==============================================================================

bool ZMQChainNotifier::Initialize()
{
    void* pcontext = nullptr;
    int major = 0, minor = 0, patch = 0;
    zmq_version(&major, &minor, &patch);
    assert(!pcontext);
    pcontext = zmq_ctx_new();

    if (!pcontext) {
        return false;
    }

    assert(!psocket);

    // check if address is being used by other publish notifier
    auto i = mapPublishNotifiers.find(address);

    if (i == mapPublishNotifiers.end()) {
        psocket = zmq_socket(pcontext, ZMQ_PUB);
        if (!psocket) {
            //            zmqError("Failed to create socket");
            return false;
        }

        //        LogPrint(BCLog::ZMQ, "zmq: Outbound message high water mark for %s at %s is %d\n",
        //        type, address, outbound_message_high_water_mark);

        int rc = zmq_setsockopt(psocket, ZMQ_SNDHWM, &outbound_message_high_water_mark,
            sizeof(outbound_message_high_water_mark));
        if (rc != 0) {
            //            zmqError("Failed to set outbound message high water mark");
            zmq_close(psocket);
            return false;
        }

        rc = zmq_bind(psocket, address.c_str());
        if (rc != 0) {
            //            zmqError("Failed to bind address");
            zmq_close(psocket);
            return false;
        }

        // register this notifier for the address, so it can be reused for other publish notifier
        mapPublishNotifiers.insert(std::make_pair(address, this));
        return true;
    } else {
        //        LogPrint(BCLog::ZMQ, "zmq: Reusing socket for address %s\n", address);
        //        LogPrint(BCLog::ZMQ, "zmq: Outbound message high water mark for %s at %s is %d\n",
        //        type, address, outbound_message_high_water_mark);

        psocket = i->second->psocket;
        mapPublishNotifiers.insert(std::make_pair(address, this));

        return true;
    }
}

//==============================================================================

void ZMQChainNotifier::Shutdown()
{
    assert(psocket);

    int count = mapPublishNotifiers.count(address);

    // remove this notifier from the list of publishers using this address
    auto iterpair = mapPublishNotifiers.equal_range(address);
    for (auto it = iterpair.first; it != iterpair.second; ++it) {
        if (it->second == this) {
            mapPublishNotifiers.erase(it);
            break;
        }
    }

    if (count == 1) {
        //        LogPrint(BCLog::ZMQ, "zmq: Close socket at address %s\n", address);
        int linger = 0;
        zmq_setsockopt(psocket, ZMQ_LINGER, &linger, sizeof(linger));
        zmq_close(psocket);
    }

    psocket = nullptr;
}

//==============================================================================

bool ZMQChainNotifier::sendMessage(const char* command, const void* data, size_t size)
{
    assert(psocket);

    /* send three parts, command & data & a LE 4byte sequence number */
    unsigned char msgseq[sizeof(uint32_t)];
    WriteLE32(&msgseq[0], nSequence);
    int rc = zmq_send_multipart(
        psocket, command, strlen(command), data, size, msgseq, (size_t)sizeof(uint32_t), nullptr);
    if (rc == -1)
        return false;

    /* increment memory only sequence number after sending */
    nSequence++;

    return true;
}

//==============================================================================

bool ZMQChainNotifier::rescan(QString startBlockHash, std::pair<QString, size_t> endBlock)
{
    if (_rescanInProgress) {
        return false;
    }

    LogCDebug(General) << "Starting rescan from block hash" << startBlockHash;

    _rescanInProgress = true;

    // TODO: Change to accept assetID
    //    _chainDataSource.getBlockHeaders(0, startBlockHash).then([this,
    //    endBlock](std::vector<BlockHeader> headers) {
    //        onBlockHeadersReceived(0, headers, endBlock);
    //    });

    return true;
}

//==============================================================================

void ZMQChainNotifier::abortRescan()
{
    LogCDebug(General) << "Aborting rescan";
    if (_rescanInProgress) {
        _rescanInProgress = false;
    }
}

//==============================================================================

bool ZMQChainNotifier::notifyBlockHeader(const Wire::VerboseBlockHeader& header)
{
    CDataStream ss(bitcoin::SER_NETWORK, bitcoin::PROTOCOL_VERSION);
    ss << bitcoin::uint256S(header.hash);
    ss << bitcoin::uint256S(header.header.prevBlock);
    ss << header.height;
    ss << static_cast<int64_t>(header.header.timestamp);
    sendMessage(MSG_BLOCKHEADER, &(*ss.begin()), ss.size());
    return true;
}

//==============================================================================

bool ZMQChainNotifier::notifyTip(const BlockHash& blockHash)
{
    auto hash = bitcoin::uint256S(blockHash.toStdString());
    char data[32];
    for (unsigned int i = 0; i < 32; i++)
        data[31 - i] = hash.begin()[i];
    sendMessage(MSG_HASHBLOCK, data, sizeof(data));
    return true;
}

//==============================================================================

// void ZMQChainNotifier::onBlockHeadersReceived(AssetID assetID,
// std::vector<Wire::VerboseBlockHeader> headers, std::pair<QString, size_t> endBlock)
//{
//    if(!_rescanInProgress)
//    {
//        return;
//    }

//    if(headers.empty())
//    {
//        _rescanInProgress = false;
//        return;
//    }

//    LogCDebug(General) << "Rescan block headers received, start_height:" << headers.front().height
//    << "end_height:" << headers.back().height;

//    for(auto &&header : headers)
//    {
//        notifyBlockHeader(header);
//        if(header.height >= endBlock.second)
//        {
//            _rescanInProgress = false;
//            return;
//        }
//    }

//    std::for_each(std::begin(headers), std::end(headers),
//    std::bind(&ZMQChainNotifier::notifyBlockHeader, this, std::placeholders::_1));

//    _chainDataSource.getBlockHeaders(assetID, headers.back().hash).then([assetID, this,
//    endBlock](std::vector<BlockHeader> headers) {
//        onBlockHeadersReceived(assetID, headers, endBlock);
//    });
//}

//==============================================================================
