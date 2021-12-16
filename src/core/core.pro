#-------------------------------------------------
#
# Project created by QtCreator 2017-08-24T14:24:54
#
#-------------------------------------------------

QT       += network concurrent

QT       -= gui

TARGET = core
TEMPLATE = lib
CONFIG += staticlib c++14

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS


INCLUDEPATH += $$PWD/vendor/include

include($$PWD/../bitcoin/libbitcoin.pri)
include($$PWD/../networking/libnetworking.pri)
include($$PWD/../../modules/libzmq/libzmq.pri)
include($$PWD/../../modules/qtpromise/qtpromise.pri)
include($$PWD/../lightningswaps/lightningswaps.pri)
include($$PWD/../lndtools/lndtools.pri)

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    Chain/RegtestChain.cpp \
    Data/AbstractOrderbookDataSource.cpp \
    Data/AllOrdersDataSource.cpp \
    Data/AssetsBalance.cpp \
    Data/CoinAsset.cpp \
    Data/LocalCurrency.cpp \
    Data/OrderBookData.cpp \
    Data/OwnOrdersDataSource.cpp \
    Data/TransactionEntry.cpp \
    Data/WalletAssetsModel.cpp \
    Factories/AbstractNetworkingFactory.cpp \
    Models/AbstractKeychain.cpp \
    Models/AllTransactionsDataSource.cpp \
    Models/AskBidListModel.cpp \
    Models/AssetsListProxyModel.cpp \
    Models/AssetTransactionsDataSource.cpp \
    Chain/ChainSyncHelper.cpp \
    Chain/ChainSyncManager.cpp \
    Models/DexService.cpp \
    Models/Context.cpp \
    Models/CurrencyModel.cpp \
    Models/EmulatorWalletDataSource.cpp \
    Models/OrderBookListModel.cpp \
    Models/OwnOrdersHistoryListModel.cpp \
    Models/OwnOrdersListModel.cpp \
    Models/SwapAssetsModel.cpp \
    Models/TradeHistoryListModel.cpp \
    Models/LightningChannelsListModel.cpp \
    Models/LightningPaymentsListModel.cpp \
    Models/TransactionsDataSource.cpp \
    Models/WalletAssetsListModel.cpp \
    Models/WalletDataSource.cpp \
    Models/WalletTransactionsListModel.cpp \
    Models/SyncStateProvider.cpp \
    ViewModels/ApplicationLndViewModel.cpp \
    ViewModels/ApplicationViewModel.cpp \
    ViewModels/EmulatorViewModel.cpp \
    ViewModels/LightningChannelBackupViewModel.cpp \
    ViewModels/LightningSendTransactionViewModel.cpp \
    ViewModels/WalletAssetViewModel.cpp \
    Models/AllTransactionsDataSource.cpp \
    Models/AssetsListProxyModel.cpp \
    Models/AssetTransactionsDataSource.cpp \
    Models/CurrencyModel.cpp \
    Models/EmulatorWalletDataSource.cpp \
    Data/Wallet.cpp \
    Models/SyncService.cpp \
    ViewModels/WalletDexViewModel.cpp \
    ViewModels/WalletViewModel.cpp \
    ViewModels/LightningViewModel.cpp \
    Factories/ChainSyncManagerFactory.cpp \
    Chain/Chain.cpp \
    Factories/ApiClientNetworkingFactory.cpp \
    Chain/BlockHeader.cpp \
    Chain/BlockIndex.cpp \
    Factories/AbstractChainSyncManagerFactory.cpp \
    Chain/AbstractChainSyncManager.cpp \
    Chain/TransactionsCache.cpp \
    Chain/AbstractTransactionsCache.cpp \
    Chain/BlockFilterMatcher.cpp \
    Chain/AbstractChainManager.cpp \
    Chain/ChainManager.cpp \
    ViewModels/ChainViewModel.cpp \
    ViewModels/LocalCurrencyViewModel.cpp \
    ViewModels/SendTransactionViewModel.cpp \
    ZeroMQ/ZMQAbstractNotifier.cpp \
    Data/SkinColors.cpp \
    ZeroMQ/ZMQChainNotifier.cpp \
    Chain/AbstractChainDataSource.cpp \
    RPC/RPCServer.cpp \
    Chain/CachedChainDataSource.cpp \
    Chain/EmulatorChainSyncManager.cpp \
    Factories/EmulatorChainSyncManagerFactory.cpp \
    RPC/GRPCServer.cpp \
    Tools/AppConfig.cpp \
    Tools/Bootstrap.cpp \
    Tools/DBUtils.cpp \
    Tools/Common.cpp \
    Models/LnDaemonInterface.cpp \
    Models/LnDaemonsManager.cpp \
    Tools/DaemonMonitor.cpp \
    Models/AutopilotModel.cpp \
    Models/AssetsRemotePriceModel.cpp \
    ViewModels/WalletDexChannelBalanceViewModel.cpp \
    Models/NotificationsModel.cpp \
    Data/NotificationData.cpp

HEADERS += \
    Chain/RegtestChain.hpp \
    Data/AbstractOrderbookDataSource.hpp \
    Data/AllOrdersDataSource.hpp \
    Data/AssetsBalance.hpp \
    Data/CoinAsset.hpp \
    Data/LocalCurrency.hpp \
    Data/OrderBookData.hpp \
    Data/OwnOrdersDataSource.hpp \
    Data/TransactionEntry.hpp \
    Data/WalletAssetsModel.hpp \
    Factories/AbstractNetworkingFactory.hpp \
    Models/AbstractKeychain.hpp \
    Models/AllTransactionsDataSource.hpp \
    Models/AskBidListModel.hpp \
    Models/AssetsListProxyModel.hpp \
    Models/AssetTransactionsDataSource.hpp \
    Chain/ChainSyncHelper.hpp \
    Chain/ChainSyncManager.hpp \
    Models/DexService.hpp \
    Models/Context.hpp \
    Models/CurrencyModel.hpp \
    Models/EmulatorWalletDataSource.hpp \
    Models/OrderBookListModel.hpp \
    Models/OwnOrdersHistoryListModel.hpp \
    Models/OwnOrdersListModel.hpp \
    Models/SwapAssetsModel.hpp \
    Models/TradeHistoryListModel.hpp \
    Models/LightningChannelsListModel.hpp \
    Models/LightningPaymentsListModel.hpp \
    Models/TransactionsDataSource.hpp \
    Models/WalletAssetsListModel.hpp \
    Models/WalletDataSource.hpp \
    Models/WalletTransactionsListModel.hpp \
    ViewModels/ApplicationLndViewModel.hpp \
    ViewModels/ApplicationViewModel.hpp \
    ViewModels/EmulatorViewModel.hpp \
    ViewModels/LightningChannelBackupViewModel.hpp \
    ViewModels/LightningSendTransactionViewModel.hpp \
    ViewModels/WalletAssetViewModel.hpp \
    Models/AllTransactionsDataSource.hpp \
    Models/AssetsListProxyModel.hpp \
    Models/AssetTransactionsDataSource.hpp \
    Models/EmulatorWalletDataSource.hpp \
    Data/Wallet.hpp \
    Models/SyncService.hpp \
    ViewModels/WalletDexViewModel.hpp \
    ViewModels/WalletViewModel.hpp \
    ViewModels/LightningViewModel.hpp \
    Factories/ChainSyncManagerFactory.hpp \
    Chain/Chain.hpp \
    Factories/ApiClientNetworkingFactory.hpp \
    Chain/BlockHeader.hpp \
    Chain/BlockIndex.hpp \
    Factories/AbstractChainSyncManagerFactory.hpp \
    Chain/AbstractChainSyncManager.hpp \
    Chain/TransactionsCache.hpp \
    Chain/AbstractTransactionsCache.hpp \
    Chain/BlockFilterMatcher.hpp \
    Chain/AbstractChainManager.hpp \
    Chain/ChainManager.hpp \
    ViewModels/ChainViewModel.hpp \
    ViewModels/LocalCurrencyViewModel.hpp \
    ViewModels/SendTransactionViewModel.hpp \
    ZeroMQ/ZMQAbstractNotifier.hpp \
    Models/SyncStateProvider.hpp \
    Data/SkinColors.hpp \
    ZeroMQ/ZMQChainNotifier.hpp \
    Chain/AbstractChainDataSource.hpp \
    RPC/RPCServer.hpp \
    Chain/CachedChainDataSource.hpp \
    Chain/EmulatorChainSyncManager.hpp \
    Factories/EmulatorChainSyncManagerFactory.hpp \
    RPC/GRPCServer.hpp \
    Tools/AppConfig.hpp \
    Tools/Bootstrap.hpp \
    Tools/DBUtils.hpp \
    Tools/Common.hpp \
    Models/LnDaemonInterface.hpp \
    Models/LnDaemonsManager.hpp \
    Tools/DaemonMonitor.hpp \
    Models/AutopilotModel.hpp \
    Models/AssetsRemotePriceModel.hpp \
    ViewModels/WalletDexChannelBalanceViewModel.hpp \
    Models/NotificationsModel.hpp \
    Data/NotificationData.hpp

unix {
    target.path = /usr/lib
    INSTALLS += target
}

#linux {
#LIBS += -lbreakpad \
#        -lbreakpad_client
#}

#INCLUDEPATH +=  $$PWD/vendor/include/breakpad

DISTFILES += \
    libcore.pri \
