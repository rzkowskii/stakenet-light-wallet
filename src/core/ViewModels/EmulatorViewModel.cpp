#include "EmulatorViewModel.hpp"
#include <Models/EmulatorWalletDataSource.hpp>
#include <Models/SyncService.hpp>
#include <ViewModels/ApplicationViewModel.hpp>

//==============================================================================

EmulatorViewModel::EmulatorViewModel(QObject* parent)
    : QObject(parent)
{
}

//==============================================================================

EmulatorViewModel::~EmulatorViewModel() {}

//==============================================================================

int EmulatorViewModel::maxHeight() const
{
    return _maxHeight;
}

//==============================================================================

void EmulatorViewModel::setMaxHeight(int newHeight)
{
    if (_maxHeight != newHeight) {
        _maxHeight = newHeight;
        maxHeightChanged();
    }
}

//==============================================================================

void EmulatorViewModel::clearTransactions(AssetID currentModel)
{
    //    if(_walletDataSource)
    //    {
    //        _walletDataSource->clearTransactions(currentModel);
    //    }
}

//==============================================================================

void EmulatorViewModel::initialize(ApplicationViewModel* applicationViewModel)
{
    _walletDataSource = qobject_cast<EmulatorWalletDataSource*>(applicationViewModel->dataSource());
}

//==============================================================================

void EmulatorViewModel::requestNewBlock(AssetID assetID, int count, QString addressTo)
{
    newBlockRequested(assetID, count, addressTo);
}

//==============================================================================

void EmulatorViewModel::addTransaction(AssetID currentModel, int count)
{
    //    if(_walletDataSource)
    //    {
    //        _walletDataSource->executeAdd(currentModel, count);
    //    }
}

//==============================================================================
