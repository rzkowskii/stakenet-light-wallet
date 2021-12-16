#include "OpenChannelViewModel.hpp"
#include <Chain/AbstractTransactionsCache.hpp>
#include <Data/WalletAssetsModel.hpp>
#include <Data/AssetsBalance.hpp>
#include <Factories/AbstractNetworkingFactory.hpp>
#include <Models/ConnextDaemonsManager.hpp>
#include <Models/DexService.hpp>
#include <Models/LnDaemonsManager.hpp>
#include <Models/OpenChannelModel.hpp>
#include <Models/OpenConnextChannelModel.hpp>
#include <Models/OpenLndChannelModel.hpp>
#include <Models/PaymentNodesManager.hpp>
#include <Models/SendAccountTransactionModel.hpp>
#include <Networking/AbstractWeb3Client.hpp>
#include <Orderbook/OrderbookClient.hpp>
#include <ViewModels/ApplicationViewModel.hpp>

//==============================================================================

OpenChannelViewModel::OpenChannelViewModel(QObject* parent)
    : QObject(parent)
{
}

//==============================================================================

OpenChannelViewModel::~OpenChannelViewModel() {}

//==============================================================================

AssetID OpenChannelViewModel::currentAssetID() const
{
    return _currentAssetID.get_value_or(-1);
}

//==============================================================================

void OpenChannelViewModel::setCurrentAssetID(int assetID)
{
    if (currentAssetID() != assetID) {
        if (assetID >= 0) {
            _currentAssetID = assetID;
            onAssetIDUpdated();
            currentAssetIDChanged();
        } else {
            _currentAssetID.reset();
        }
    }
}

//==============================================================================

void OpenChannelViewModel::initialize(ApplicationViewModel* applicationViewModel)
{
    _applicationViewModel = applicationViewModel;
    onAssetIDUpdated();
}

//==============================================================================

void OpenChannelViewModel::createOpenChannelRequest(
    QString identityKey, QString localAmount, unsigned feeSatsPerByte)
{
    _openChannelModel->createOpenChannelRequest(identityKey, localAmount, feeSatsPerByte);
}

//==============================================================================

void OpenChannelViewModel::cancelRequest()
{
    _openChannelModel->cancelRequest();
}

//==============================================================================

void OpenChannelViewModel::confirmRequest()
{
    _openChannelModel->confirmRequest();
}

//==============================================================================

void OpenChannelViewModel::onAssetIDUpdated()
{
    if (!_applicationViewModel || !_currentAssetID) {
        return;
    }
    initOpenChannelModel();
}

//==============================================================================

void OpenChannelViewModel::initOpenChannelModel()
{
    Q_ASSERT_X(
        _currentAssetID.has_value(), __FUNCTION__, "Expected to have asset ID at this point");

    if (_openChannelModel) {
        disconnect(_openChannelModel, nullptr, this, nullptr);
        _openChannelModel->deleteLater();
    }

    auto assetID = currentAssetID();
    auto assetsModel = _applicationViewModel->assetsModel();
    auto assetsBalance = _applicationViewModel->assetsBalance();
    const auto& asset = assetsModel->assetById(assetID);

    auto createConnextModel = [this, assetID, assetsModel, assetsBalance, asset]() -> Promise<OpenChannelModel*> {
        return _applicationViewModel->transactionsCache()->cacheById(assetID).then(
            [assetID, asset, assetsModel, assetsBalance, this](AbstractTransactionsCache* cache) {
                // create send tx model
                auto chainId = *asset.params().chainId;
                auto factory = _applicationViewModel->apiClientsFactory();
                auto web3 = factory->createWeb3Client(chainId);
                auto sendTransactionModel = new SendAccountTransactionModel{ assetID, assetsModel,
                    _applicationViewModel->accountDataSource(), std::move(web3), cache, this };

                auto orderbook = _applicationViewModel->dexService()->orderbook();

                auto connextDaemonManager
                    = _applicationViewModel->paymentNodesManager()->connextDaemonManager();
                OpenChannelModel* openChannelModel = new OpenConnextChannelModel(assetID,
                    assetsModel, assetsBalance, sendTransactionModel, connextDaemonManager->interfaceById(assetID),
                    orderbook->feeRefunding(), this);
                return openChannelModel;
            });
    };
    auto createLndModel = [this, assetID, assetsModel]() -> Promise<OpenChannelModel*> {
        auto lnDaemonManager = _applicationViewModel->paymentNodesManager()->lnDaemonManager();
        OpenChannelModel* openChannelModel = new OpenLndChannelModel(
            assetID, assetsModel, lnDaemonManager->interfaceById(assetID), this);
        return QtPromise::resolve(openChannelModel);
    };

    auto createModel
        = asset.type() == CoinAsset::Type::Account ? createConnextModel() : createLndModel();

    createModel.tap([this](OpenChannelModel* model) {
        _openChannelModel = model;
        connect(_openChannelModel, &OpenChannelModel::channelOpened, this,
            &OpenChannelViewModel::channelOpened);
        connect(_openChannelModel, &OpenChannelModel::channelOpeningFailed, this,
            &OpenChannelViewModel::channelOpeningFailed);
        connect(_openChannelModel, &OpenChannelModel::requestCreated, this,
            &OpenChannelViewModel::requestCreated);
        connect(_openChannelModel, &OpenChannelModel::requestCreatingFailed, this,
            &OpenChannelViewModel::requestCreatingFailed);
    });
}

//==============================================================================
