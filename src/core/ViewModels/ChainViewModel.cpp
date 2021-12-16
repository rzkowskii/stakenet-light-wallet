#include "ChainViewModel.hpp"
#include <Chain/AbstractChainManager.hpp>
#include <Chain/Chain.hpp>
#include <Utils/Logging.hpp>
#include <ViewModels/ApplicationViewModel.hpp>

#include <QTimer>

//==============================================================================

ChainViewModel::ChainViewModel(QObject* parent)
    : QObject(parent)
{
    _updateTimer = new QTimer(this);
    _updateTimer->setInterval(2000);
    _updateTimer->setSingleShot(false);
    connect(_updateTimer, &QTimer::timeout, this, &ChainViewModel::updateBlockHeight);
    _updateTimer->start(_updateTimer->interval());
}

//==============================================================================

unsigned ChainViewModel::chainHeight() const
{
    return _chainHeight;
}

//==============================================================================

AssetID ChainViewModel::assetID() const
{
    return _assetID.get_value_or(-1);
}

//==============================================================================

void ChainViewModel::setAssetID(AssetID assetId)
{
    if (assetID() != assetId) {
        _assetID = assetId;
        assetIDChanged();
        initChainView();
    }
}

//==============================================================================

void ChainViewModel::initialize(ApplicationViewModel* appViewModel)
{
    _chainManager = appViewModel->chainManager();
    initChainView();
}

//==============================================================================

void ChainViewModel::updateBlockHeight()
{
    if (_chainView) {
        _chainView->chainHeight().then([=](size_t newHeight) {
            _chainHeight = newHeight;
            updated();
        });
    }
}

//==============================================================================

void ChainViewModel::initChainView()
{
    if (_assetID && _chainManager) {
        auto id = assetID();
        _chainManager
            ->getChainView(id, AbstractChainManager::ChainViewUpdatePolicy::CompressedEvents)
            .then([=](std::shared_ptr<ChainView> chainView) {
                _chainView.swap(chainView);
                updateBlockHeight();
            })
            .fail([this, id] {
                //            LogCDebug(General) << "ChainViewModel::initialize, chain not found" <<
                //            id;
            });
    }
}

//==============================================================================
