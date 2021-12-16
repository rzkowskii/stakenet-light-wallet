#include "LockingViewModel.hpp"
#include <Models/WalletDataSource.hpp>
#include <ViewModels/ApplicationViewModel.hpp>

#include <support/allocators/secure.h>

//==============================================================================

LockingViewModel::LockingViewModel(WalletDataSource& dataSource, QObject* parent)
    : QObject(parent)
    , _walletDataSource(dataSource)
{
    connect(&_walletDataSource, &WalletDataSource::isWalletCryptedChanged, this,
        &LockingViewModel::isEncryptedChanged);
}

//==============================================================================

bool LockingViewModel::isEncrypted() const
{
    return _walletDataSource.isEncrypted();
}

//==============================================================================

void LockingViewModel::lock()
{
    QTimer::singleShot(0, [this] {
        ApplicationViewModel::Instance()->destroyContext();
        ApplicationViewModel::Instance()->loadContext();
    });
}

//==============================================================================
