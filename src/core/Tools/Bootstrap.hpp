#ifndef BOOTSTRAP_HPP
#define BOOTSTRAP_HPP

#include <Chain/BlockIndex.hpp>
#include <QObject>
#include <Tools/Common.hpp>
#include <queue>
#include <txdb.h>

class QFileStream;

//==============================================================================

/*!
 * \brief The BlockIndexProvider class can be used to get block indexes in order, from oldest to
 * newest Gives blocks in one way order, controlled by remainingBlocks
 */
class BlockIndexProvider {
public:
    explicit BlockIndexProvider(std::vector<BlockHash> blockHashes, AssetID assetID);
    virtual std::vector<bitcoin::CDiskBlockIndex> getBlockIndexes() = 0;
    size_t remainingBlocks() const;

    virtual ~BlockIndexProvider();

protected:
    std::vector<BlockHash> _blockHashes;
    AssetID _assetID;
    size_t _readBlocks{ 0 };
};

//==============================================================================

struct BlockIndexProviderFactory {
    virtual std::unique_ptr<BlockIndexProvider> create(AssetID assetID) = 0;
    virtual ~BlockIndexProviderFactory();
};

//==============================================================================

class BootstrapBuilder : public QObject {
public:
    explicit BootstrapBuilder(QString filePath,
        BlockIndexProviderFactory& blockIndexProviderFactory, std::vector<AssetID> assets,
        QObject* parent = nullptr);
    ~BootstrapBuilder();

    AssetID currentAssetID() const;
    bool canBuildNext() const;
    void build();
    void finalize();

private:
    BlockIndexProviderFactory& _blockIndexProviderFactory;
    std::queue<AssetID> _queue;
    std::unique_ptr<QFileStream> _stream;
    QString _filePath;
};

//==============================================================================

class BootstrapReader : public QObject {
public:
    explicit BootstrapReader(QString filePath);
    ~BootstrapReader() override;
    bool canReadMore() const;
    void finalize();

    struct ReaderContext {
        explicit ReaderContext(AssetID assetID);

        virtual ~ReaderContext() = default;
        virtual bool canReadMore() const = 0;
        virtual bitcoin::CDiskBlockIndex readNext() = 0;
        virtual size_t remainingBlocks() const = 0;

        AssetID assetID;
    };

    std::unique_ptr<ReaderContext> readNextAsset();

private:
    std::unique_ptr<QFileStream> _stream;
    QString _filePath;
};

//==============================================================================

class BootstrapWriter : public QObject, public BlockIndexProviderFactory {
    Q_OBJECT
public:
    explicit BootstrapWriter(QObject* parent = nullptr);
    ~BootstrapWriter() override;
    std::unique_ptr<BootstrapBuilder> createBuilder(QString filePath, std::vector<AssetID> assets);
    std::unique_ptr<BlockIndexProvider> create(AssetID assetID) override;

signals:

public slots:

private:
    bitcoin::CBlockTreeDB& db();
    std::vector<BlockHash> readBlockHashes(AssetID assetID);

private:
    std::unique_ptr<bitcoin::CBlockTreeDB> _blockTreeDb;
};

//==============================================================================

#endif // BOOTSTRAP_HPP
