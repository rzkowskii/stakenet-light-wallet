#include "WalletDataSource.hpp"
#include <QTimer>

//==============================================================================

WalletDataSource::WalletDataSource(QObject* parent)
    : QObject(parent)
{
}

//==============================================================================

WalletDataSource::~WalletDataSource() {}

//==============================================================================

Promise<QByteArray> WalletDataSource::dumpPrivKey(
    AssetID assetID, std::vector<unsigned char> scriptPubKey) const
{
    return QtPromise::resolve(QByteArray());
}

//==============================================================================

UTXOSetDataSource::~UTXOSetDataSource() {}

//==============================================================================

AccountDataSource::~AccountDataSource() {}

//==============================================================================
