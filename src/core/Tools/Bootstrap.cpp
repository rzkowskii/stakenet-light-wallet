#include "Bootstrap.hpp"
#include <Tools/Common.hpp>
#include <Tools/DBUtils.hpp>
#include <Utils/Logging.hpp>
#include <serialize.h>
#include <streams.h>
#include <txdb.h>

//==============================================================================

static constexpr int BOOTSTRAP_VERSION = 1;

//==============================================================================

class QFileStream {

public:
    QFileStream(int nType, int nVersion)
        : nType(nType)
        , nVersion(nVersion)
    {
    }
    //
    // Stream subset
    //
    int GetType() const { return nType; }
    int GetVersion() const { return nVersion; }

    void read(char* pch, size_t nSize)
    {
        if (!_file.isOpen())
            throw std::ios_base::failure("QFileStream::read: file handle is nullptr");

        if (_file.read(pch, nSize) != nSize)
            throw std::ios_base::failure(_file.atEnd() ? "QFileStream::read: end of file"
                                                       : "QFileStream::read: fread failed");
    }

    void open(QString filePath, QFile::OpenModeFlag mode)
    {
        _file.setFileName(filePath);
        if (!_file.open(mode)) {
            throw std::ios_base::failure("Failed to open file");
        }
    }

    bool isAtEnd() const { return _file.atEnd(); }

    void move(QString newName)
    {
        if (!_file.rename(newName)) {
            throw std::ios_base::failure(
                "QFileStream::move, failed to move file to location " + newName.toStdString());
        }
    }

    void ignore(size_t nSize)
    {
        if (!_file.isOpen())
            throw std::ios_base::failure("QFileStream::ignore: file handle is nullptr");
        char data[4096];
        while (nSize > 0) {
            size_t nNow = std::min<size_t>(nSize, sizeof(data));
            if (_file.read(data, nNow) != nNow)
                throw std::ios_base::failure(_file.atEnd() ? "QFileStream::ignore: end of file"
                                                           : "QFileStream::read: fread failed");
            nSize -= nNow;
        }
    }

    void write(const char* pch, size_t nSize)
    {
        if (!_file.isOpen())
            throw std::ios_base::failure("QFileStream::write: file handle is nullptr");
        if (_file.write(pch, nSize) != nSize)
            throw std::ios_base::failure("QFileStream::write: write failed");
    }

    template <typename T> QFileStream& operator<<(const T& obj)
    {
        // Serialize to this stream
        if (!_file.isOpen())
            throw std::ios_base::failure("QFileStream::operator<<: file handle is nullptr");
        bitcoin::Serialize(*this, obj);
        return (*this);
    }

    template <typename T> QFileStream& operator>>(T&& obj)
    {
        // Unserialize from this stream
        if (!_file.isOpen())
            throw std::ios_base::failure("QFileStream::operator>>: file handle is nullptr");
        bitcoin::Unserialize(*this, obj);
        return (*this);
    }

private:
    int nType;
    int nVersion;
    QFile _file;
};

//==============================================================================

BootstrapWriter::BootstrapWriter(QObject* parent)
    : QObject(parent)
{
}

//==============================================================================

BootstrapWriter::~BootstrapWriter() {}

//==============================================================================

std::unique_ptr<BootstrapBuilder> BootstrapWriter::createBuilder(
    QString filePath, std::vector<AssetID> assets)
{
    return std::unique_ptr<BootstrapBuilder>(new BootstrapBuilder(filePath, *this, assets));
}

//==============================================================================

std::unique_ptr<BlockIndexProvider> BootstrapWriter::create(AssetID assetID)
{

#if 0
    struct BlockIndexProviderImpl : public BlockIndexProvider {
        explicit BlockIndexProviderImpl(std::vector<BlockHash> blockHashes, AssetID assetID, bitcoin::CBlockTreeDB &blockTreeDb) :
            BlockIndexProvider(blockHashes, assetID),
            _blockTreeDb(blockTreeDb)
        { }
        virtual std::vector<bitcoin::CDiskBlockIndex> getBlockIndexes()
        {
            std::vector<bitcoin::CDiskBlockIndex> result;
            auto blocksToRead = std::min(remainingBlocks(), size_t(500));

            for(size_t i = 0; i < blocksToRead; ++i)
            {
                auto hash = _blockHashes.at(_readBlocks++);
                bitcoin::CDiskBlockIndex diskIndex;
                if(!_blockTreeDb.ReadBlockIndex(_assetID, bitcoin::uint256S(hash.toStdString()), diskIndex))
                {
                    throw std::runtime_error(
                                QString("Failed to read block index from db, shouldn't happen, hash: %1, height: %2")
                                .arg(hash).arg(_readBlocks - 1).toStdString());
                }

                result.push_back(diskIndex);
            }

            return result;
        }

        bitcoin::CBlockTreeDB &_blockTreeDb;
    };

    return std::unique_ptr<BlockIndexProvider>(new BlockIndexProviderImpl(readBlockHashes(assetID), assetID, db()));
#endif
    throw std::runtime_error("No implemented");
}

//==============================================================================

bitcoin::CBlockTreeDB& BootstrapWriter::db()
{
    if (!_blockTreeDb) {
        _blockTreeDb.reset(new bitcoin::CBlockTreeDB(GetChainIndexDir(false).toStdString(), 10000));
    }
    return *_blockTreeDb;
}

//==============================================================================

std::vector<BlockHash> BootstrapWriter::readBlockHashes(AssetID assetID)
{
#if 0
    std::pair<bitcoin::uint256, uint64_t> hashBestBlock;
    std::vector<BlockHash> result;
    if(db().ReadBestBlock(assetID, hashBestBlock))
    {
        auto readIndex = [this, assetID](bitcoin::uint256 blockHash) {
            bitcoin::CDiskBlockIndex diskIndex;
            bool result = db().ReadBlockIndex(assetID, blockHash, diskIndex);
            return std::make_tuple(diskIndex, result);
        };

        bitcoin::uint256 prevIndexHash = hashBestBlock.first;
        int lastHeight = -1;
        while(!prevIndexHash.IsNull())
        {
            result.push_back(QString::fromStdString(prevIndexHash.ToString()));

            bitcoin::CDiskBlockIndex diskIndex;
            bool success;
            std::tie(diskIndex, success) = readIndex(prevIndexHash);
            if(!success)
            {
                break;
            }

            if(lastHeight > 0 && diskIndex.nHeight != (lastHeight - 1))
            {
                throw std::runtime_error("Unordered chain");
            }

            lastHeight = diskIndex.nHeight;
            prevIndexHash = diskIndex.hashPrevBlock;
        }

        std::reverse(std::begin(result), std::end(result));
    }

    return result;
#endif
    return {};
}

//==============================================================================

BootstrapBuilder::BootstrapBuilder(QString filePath,
    BlockIndexProviderFactory& blockIndexProviderFactory, std::vector<AssetID> assets,
    QObject* parent)
    : QObject(parent)
    , _blockIndexProviderFactory(blockIndexProviderFactory)
{
    for (auto&& asset : assets) {
        _queue.push(asset);
    }

    if (canBuildNext()) {
        _stream.reset(new QFileStream(bitcoin::SER_DISK, BOOTSTRAP_VERSION));
        QFileInfo targetFileInfo(filePath);
        targetFileInfo.makeAbsolute();
        _filePath = targetFileInfo.absoluteFilePath();
        auto tempPath = _filePath;
        auto whereToReplace = tempPath.lastIndexOf(targetFileInfo.baseName());
        tempPath = tempPath.replace(whereToReplace, targetFileInfo.baseName().length(),
            QString("%1_temp").arg(targetFileInfo.baseName()));
        _stream->open(tempPath, QFile::WriteOnly);
    }
}

//==============================================================================

BootstrapBuilder::~BootstrapBuilder() {}

//==============================================================================

AssetID BootstrapBuilder::currentAssetID() const
{
    return _queue.front();
}

//==============================================================================

bool BootstrapBuilder::canBuildNext() const
{
    return !_queue.empty();
}

//==============================================================================

void BootstrapBuilder::build()
{
    if (!canBuildNext()) {
        return;
    }

    QString bestBlockHash;

    auto blockIndexProvider = _blockIndexProviderFactory.create(currentAssetID());
    LogCDebug(General) << "Starting building for asset" << currentAssetID()
                       << "blocks: " << blockIndexProvider->remainingBlocks();

    if (blockIndexProvider->remainingBlocks() > 0) {
        (*_stream) << std::make_pair<uint32_t, uint32_t>(
            currentAssetID(), blockIndexProvider->remainingBlocks());

        while (blockIndexProvider->remainingBlocks() > 0) {
            for (auto&& index : blockIndexProvider->getBlockIndexes()) {
                (*_stream) << index;
            }
        }
    }

    LogCDebug(General) << "Finished building for asset" << currentAssetID();
    _queue.pop();
}

//==============================================================================

void BootstrapBuilder::finalize()
{
    if (QFileInfo(_filePath).exists()) {
        QFile::remove(_filePath);
    }

    _stream->move(_filePath);
    LogCDebug(General) << "Finished building of bootstrap file, path:" << _filePath;
}

//==============================================================================

BlockIndexProvider::BlockIndexProvider(std::vector<BlockHash> blockHashes, AssetID assetID)
    : _blockHashes(blockHashes)
    , _assetID(assetID)
{
}

//==============================================================================

size_t BlockIndexProvider::remainingBlocks() const
{
    return _blockHashes.size() - _readBlocks;
}

//==============================================================================

BlockIndexProvider::~BlockIndexProvider() {}

//==============================================================================

BlockIndexProviderFactory::~BlockIndexProviderFactory() {}

//==============================================================================

BootstrapReader::BootstrapReader(QString filePath)
    : _filePath(filePath)
{
    _stream.reset(new QFileStream(bitcoin::SER_DISK, BOOTSTRAP_VERSION));
    _stream->open(_filePath, QFile::ReadOnly);
}

//==============================================================================

BootstrapReader::~BootstrapReader() {}

//==============================================================================

bool BootstrapReader::canReadMore() const
{
    return !_stream->isAtEnd();
}

//==============================================================================

void BootstrapReader::finalize()
{
    _stream.reset();
    QFile::rename(_filePath, _filePath + ".old");
}

//==============================================================================

std::unique_ptr<BootstrapReader::ReaderContext> BootstrapReader::readNextAsset()
{
    std::pair<uint32_t, uint32_t> assetEntry;
    (*_stream) >> assetEntry;

    struct ReaderContextImpl : public ReaderContext {
        ReaderContextImpl(AssetID assetID, size_t blockCount, BootstrapReader& reader)
            : ReaderContext(assetID)
            , blockCount(blockCount)
            , reader(reader)
        {
        }
        bool canReadMore() const override { return blockCount > 0; }
        bitcoin::CDiskBlockIndex readNext() override
        {
            bitcoin::CDiskBlockIndex diskIndex;
            if (canReadMore()) {
                (*reader._stream) >> diskIndex;
                --blockCount;
            }

            return diskIndex;
        }

        size_t remainingBlocks() const override { return blockCount; }

        size_t blockCount;
        BootstrapReader& reader;
    };

    return std::unique_ptr<ReaderContext>(
        new ReaderContextImpl(assetEntry.first, assetEntry.second, *this));
}

//==============================================================================

BootstrapReader::ReaderContext::ReaderContext(AssetID assetID)
    : assetID(assetID)
{
}
