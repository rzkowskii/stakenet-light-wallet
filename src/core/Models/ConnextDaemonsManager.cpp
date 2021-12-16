#include "ConnextDaemonsManager.hpp"
#include <Chain/TransactionsCache.hpp>
#include <Chain/AbstractTransactionsCache.hpp>
#include <Data/WalletAssetsModel.hpp>
#include <Data/ConnextPaymentsProxy.hpp>
#include <Factories/ApiClientNetworkingFactory.hpp>
#include <LndTools/ConnextBrowserNodeApi.hpp>
#include <LndTools/ConnextHttpClient.hpp>
#include <LndTools/ConnextProcessManager.hpp>
#include <Models/ConnextDaemonInterface.hpp>
#include <Models/SendAccountTransactionModel.hpp>
#include <Models/WalletDataSource.hpp>
#include <Networking/AbstractWeb3Client.hpp>
#include <Networking/DockerApiClient.hpp>
#include <Networking/LocalSocketRequestHandlerImpl.hpp>
#include <Networking/NetworkingUtils.hpp>
#include <Networking/RequestHandlerImpl.hpp>
#include <Swaps/ConnextSwapClient.hpp>
#include <Utils/Logging.hpp>
#include <ViewModels/ApplicationViewModel.hpp>

#include <QNetworkAccessManager>

//==============================================================================

ConnextDaemonsManager::ConnextDaemonsManager(Utils::WorkerThread& workerThread,
    WalletAssetsModel& assetsModel, AccountDataSource& dataSource,
    AbstractNetworkingFactory& apiClientFactory, QPointer<AssetsTransactionsCache> txCache,
    QObject* parent)
    : QObject(parent)
    , _workerThread(workerThread)
    , _assetsModel(assetsModel)
    , _accountDataSource(dataSource)
    , _apiClientFactory(apiClientFactory)
    , _txCache(txCache)
{
    init();
}

//==============================================================================

ConnextDaemonsManager::~ConnextDaemonsManager() {}

//==============================================================================

ConnextBrowserNodeApi* ConnextDaemonsManager::browserNodeApi() const
{
    return _browserNodeApi;
}

//==============================================================================

ConnextDaemonInterface* ConnextDaemonsManager::interfaceById(AssetID assetID) const
{
    return _daemonsInterfaces.count(assetID) > 0 ? _daemonsInterfaces.at(assetID).get() : nullptr;
}

//==============================================================================

ConnextProcessManager* ConnextDaemonsManager::processManager() const
{
    return _processManager.get();
}

//==============================================================================

void ConnextDaemonsManager::init()
{
    auto connextConfig = AppConfig::Instance().config().connextConfig.clientConfig;
    _processManager.reset(new ConnextProcessManager(connextConfig));
    registerConnextConf(connextConfig);

    for (auto assetID : _assetsModel.accountAssets()) {
            qobject_delete_later_unique_ptr<ConnextDaemonInterface> interface(
                new ConnextDaemonInterface(_browserNodeApi, _processManager.get(), _assetsModel, assetID));
            _daemonsInterfaces.emplace(assetID, std::move(interface));
            _txCache->cacheById(assetID).then(
                [this, assetID](AbstractTransactionsCache* cache) {
                    qobject_delete_later_unique_ptr<ConnextPaymentsProxy> paymentManager(
                        new ConnextPaymentsProxy(assetID, cache ,_browserNodeApi, _assetsModel ));
                    _paymentManagers.emplace(assetID, std::move(paymentManager));
                });
    }
}

//==============================================================================

void ConnextDaemonsManager::registerConnextConf(ConnextDaemonConfig config)
{
    Q_ASSERT(_processManager);
    RequestHandlerImpl::Domains domains;
    domains.normal = domains.tor = QString("%1:%2").arg(config.host).arg(config.port);
    static QNetworkAccessManager accessManager;
    std::unique_ptr<RequestHandlerImpl> requestHandler(
        new RequestHandlerImpl(&accessManager, nullptr, domains));

    _connextInitTimer = new QTimer{ this };
    connect(_connextInitTimer, &QTimer::timeout, this, &ConnextDaemonsManager::tryInitNode);
    connect(_processManager.get(), &ConnextProcessManager::runningChanged, this, [this] {
        if (_processManager->running()) {
            _connextInitTimer->start();
        } else {
            _connextInitTimer->stop();
            setNodeInitialized(false);
        }
    });

    _connextInitTimer->setSingleShot(false);
    _connextInitTimer->setInterval(1000);

    auto sendTxDelegate = [this](AbstractConnextApi::DepositTxParams params) -> Promise<void> {
        Q_ASSERT_X(thread() == QThread::currentThread(), __FUNCTION__,
            "Calling sync method from different thread");

        AssetID assetID = params.assetID;
        const auto& asset = _assetsModel.assetById(assetID);
        int64_t amountSats
            = eth::ConvertDenominations(static_cast<eth::u256>(params.amount.toDouble()),
                UNITS_PER_CURRENCY.at(asset.coinID()), 8)
                  .convert_to<int64_t>();
        double amount = static_cast<double>(amountSats) / COIN;
        eth::AccountSendTxParams sendParams(
            assetID, params.channelAddress, amount, {}, {}, {}, {}, Enums::GasType::Average);

        return _txCache->cacheById(assetID).then([this, sendParams, assetID,
                                                     chainId = params.chainId](
                                                     AbstractTransactionsCache* txCache) {
            auto web3 = _apiClientFactory.createWeb3Client(chainId);
            auto web3Raw = web3.get();
            auto sendTxModel = new SendAccountTransactionModel(
                assetID, &_assetsModel, &_accountDataSource, std::move(web3), txCache, this);
            // Stupid Cpp doesn't allow to move in unique ptr into this labmda.
            // We need to create Web3 client, then wait for it to be connected and only after
            // we can try to send transactions.
            return QtPromise::connect(web3Raw, &AbstractWeb3Client::connected)
                .then([this, assetID, sendParams, sendTxModel] {
                    return _accountDataSource.getAccountAddress(assetID).then(
                        [sendParams, sendTxModel](QString address) {
                            return sendTxModel->createSignedSendTransaction(sendParams, address)
                                .then([address, sendTxModel](eth::SignedTransaction tx) {
                                    return sendTxModel->sendTransaction(address, tx)
                                        .then([] {})
                                        .fail([]() {
                                            return Promise<void>::reject(std::current_exception());
                                        });
                                });
                        });
                })
                .finally([sendTxModel] { sendTxModel->deleteLater(); });
        });
    };

    _browserNodeApi = new ConnextBrowserNodeApi(sendTxDelegate, this);
}

//==============================================================================

void ConnextDaemonsManager::tryInitNode()
{
    if (!_nodeInitialized) {
        if (_processManager->running()) {
            _connextInitTimer->stop();
            _accountDataSource.dumpPrivKey(60).then([this](QString mnemonic) {
                _connextInitTimer->stop();
                _browserNodeApi->initNode(mnemonic)
                    .then([this]() { setNodeInitialized(true); })
                    .fail([this](const std::exception& ex) {
                        _connextInitTimer->start();
                        LogCCritical(Connext)
                            << "Could not initialize connect browser node:" << ex.what();
                    });
            });
        }
    }
}

//==============================================================================

void ConnextDaemonsManager::setNodeInitialized(bool value)
{
    if (_nodeInitialized != value) {
        _nodeInitialized = value;
        for (const auto& [_, interface] : _daemonsInterfaces) {
            interface->setNodeInitialized(_nodeInitialized);
        }
    }
}

//==============================================================================
