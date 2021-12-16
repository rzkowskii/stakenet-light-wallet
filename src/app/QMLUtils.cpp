#include "QMLUtils.hpp"
#include <AppUpdater.hpp>
#include <Data/SkinColors.hpp>
#include <LndTools/ConnextBrowserNodeApi.hpp>
#include <Models/AssetsListProxyModel.hpp>
#include <Models/ChannelRentingModel.hpp>
#include <Models/ConnextStateModel.hpp>
#include <Models/CurrencyModel.hpp>
#include <Models/DexRefundableFeeModel.hpp>
#include <Models/LnDaemonStateModel.hpp>
#include <Models/NotificationsModel.hpp>
#include <Models/OrderBookListModel.hpp>
#include <Models/OwnOrdersHistoryListModel.hpp>
#include <Models/OwnOrdersListModel.hpp>
#include <Models/PaymentChannelsListModel.hpp>
#include <Models/PaymentNodeStateModel.hpp>
#include <Models/SwapAssetsModel.hpp>
#include <Models/SyncStateProvider.hpp>
#include <Models/TradeHistoryListModel.hpp>
#include <Models/TradingBotModel.hpp>
#include <Models/WalletAssetsListModel.hpp>
#include <Models/WalletDexStateModel.hpp>
#include <Models/WalletTransactionsListModel.hpp>
#include <MouseEventSpy.hpp>
#include <QMLClipboardAdapter.hpp>
#include <StakenetConfig.hpp>
#include <Tools/Common.hpp>
#include <TorManager.hpp>
#include <ViewModels/ApplicationPaymentNodesViewModel.hpp>
#include <ViewModels/ApplicationViewModel.hpp>
#include <ViewModels/ChainViewModel.hpp>
#include <ViewModels/ConnextBrowserNodeProxyViewModel.hpp>
#include <ViewModels/EmulatorViewModel.hpp>
#include <ViewModels/LightningChannelBackupViewModel.hpp>
#include <ViewModels/LightningSendTransactionViewModel.hpp>
#include <ViewModels/LocalCurrencyViewModel.hpp>
#include <ViewModels/LockingViewModel.hpp>
#include <ViewModels/OpenChannelViewModel.hpp>
#include <ViewModels/PaymentNodeViewModel.hpp>
#include <ViewModels/SendTransactionViewModel.hpp>
#include <ViewModels/WalletAssetViewModel.hpp>
#include <ViewModels/WalletDexChannelBalanceViewModel.hpp>
#include <ViewModels/WalletDexViewModel.hpp>
#include <ViewModels/WalletMarketSwapViewModel.hpp>
#include <ViewModels/WalletViewModel.hpp>

#include <QFontDatabase>
#include <QQmlEngine>
#include <array>
#include <cmath>

#define QML_REGISTER_TYPE_HELPER(Class) qmlRegisterType<Class>(uri, 1, 0, #Class)
#define QML_REGISTER_UNCREATABLE_TYPE_HELPER(Class)                                                \
    qmlRegisterUncreatableType<Class>(uri, 1, 0, #Class, #Class " Uncreatable type")

static const QString CHANGE_LOG_FILE_PATH(":/data/CHANGELOG.txt");
static const int HOURS_PER_DAY = 24;

//==============================================================================

void QMLUtils::RegisterQMLTypes()
{
    RegisterModels();
    RegisterViewModels();
    RegisterFonts();
    RegisterUtils();
}

//==============================================================================

void QMLUtils::setContextProperties(QQmlContext* context)
{
    QMLUtils::Sizes sizes;
    std::array<std::pair<QString, QVariant>, 16> sizeValues = { {
        { "menuWidthSmallMode", sizes.menuWidthSmallMode },
        { "windowWidthSmallMode", sizes.windowWidthSmallMode },

        { "assetsViewWidthSmallMode", sizes.assetsViewWidthSmallMode },
        { "assetsViewWidthLargeMode", sizes.assetsViewWidthLargeMode },

        { "headerViewHeightSmallMode", sizes.headerViewHeightSmallMode },
        { "headerViewHeightMediumMode", sizes.headerViewHeightMediumMode },
        { "headerViewHeightLargeMode", sizes.headerViewHeightLargeMode },

        { "coinsSizeSmallMode", sizes.coinsSizeSmallMode },
        { "coinsSizeMediumMode", sizes.coinsSizeMediumMode },
        { "coinsSizeLargeMode", sizes.coinsSizeLargeMode },

        { "menuItemHeightSmallMode", sizes.menuItemHeightSmallMode },

        { "closedTransactionHeight", sizes.closedTransactionHeight },
        { "openedTransactionHeight", sizes.openedTransactionHeight },
    } };

    for (auto property : sizeValues) {
        context->setContextProperty(property.first, property.second);
    }

    context->setContextProperty("isMobile", ApplicationViewModel::IsMobile());
    context->setContextProperty("isStaging", ApplicationViewModel::IsStagingEnv());
    context->setContextProperty("buildHash", QString(STAKENET_GIT_SHA1_BUILD));
    context->setContextProperty("appVersion", StakenetVersion());
}

//==============================================================================

void QMLUtils::RegisterViewModels()
{
#define uri "com.xsn.viewmodels"
    QML_REGISTER_TYPE_HELPER(WalletAssetViewModel);
    QML_REGISTER_TYPE_HELPER(EmulatorViewModel);
    QML_REGISTER_TYPE_HELPER(ChainViewModel);
    QML_REGISTER_TYPE_HELPER(SendTransactionViewModel);
    QML_REGISTER_TYPE_HELPER(OpenChannelViewModel);
    QML_REGISTER_TYPE_HELPER(LightningSendTransactionViewModel);
    QML_REGISTER_UNCREATABLE_TYPE_HELPER(WalletViewModel);
    QML_REGISTER_TYPE_HELPER(PaymentNodeViewModel);
    QML_REGISTER_UNCREATABLE_TYPE_HELPER(LocalCurrencyViewModel);
    QML_REGISTER_UNCREATABLE_TYPE_HELPER(SyncStateProvider);
    QML_REGISTER_TYPE_HELPER(WalletDexViewModel);
    QML_REGISTER_TYPE_HELPER(LightningChannelBackupViewModel);
    QML_REGISTER_TYPE_HELPER(WalletDexChannelBalanceViewModel);
    QML_REGISTER_UNCREATABLE_TYPE_HELPER(ApplicationPaymentNodesViewModel);
    QML_REGISTER_TYPE_HELPER(WalletMarketSwapViewModel);
    QML_REGISTER_TYPE_HELPER(ConnextBrowserNodeProxyViewModel);
    QML_REGISTER_UNCREATABLE_TYPE_HELPER(LockingViewModel);

    qmlRegisterSingletonType<ApplicationViewModel>(
        uri, 1, 0, "ApplicationViewModel", [](QQmlEngine* engine, QJSEngine*) -> QObject* {
            engine->setObjectOwnership(ApplicationViewModel::Instance(), QQmlEngine::CppOwnership);
            return ApplicationViewModel::Instance();
        });
}

//==============================================================================

void QMLUtils::RegisterModels()
{
#undef uri
#define uri "com.xsn.models"
    QML_REGISTER_TYPE_HELPER(WalletAssetsListModel);
    QML_REGISTER_TYPE_HELPER(QMLSortFilterListProxyModel);
    QML_REGISTER_TYPE_HELPER(CurrencyModel);
    QML_REGISTER_TYPE_HELPER(SwapAssetsModel);
    QML_REGISTER_TYPE_HELPER(NotificationsModel);
    QML_REGISTER_TYPE_HELPER(ChannelRentingModel);
    QML_REGISTER_TYPE_HELPER(DexRefundableFeeModel);
    QML_REGISTER_UNCREATABLE_TYPE_HELPER(PaymentChannelsListModel);
    QML_REGISTER_UNCREATABLE_TYPE_HELPER(OrderBookListModel);
    QML_REGISTER_UNCREATABLE_TYPE_HELPER(OwnOrdersListModel);
    QML_REGISTER_UNCREATABLE_TYPE_HELPER(OwnOrdersHistoryListModel);
    QML_REGISTER_UNCREATABLE_TYPE_HELPER(WalletTransactionsListModel);
    QML_REGISTER_UNCREATABLE_TYPE_HELPER(LnDaemonStateModel);
    QML_REGISTER_UNCREATABLE_TYPE_HELPER(ConnextStateModel);
    QML_REGISTER_UNCREATABLE_TYPE_HELPER(WalletDexStateModel);
    QML_REGISTER_UNCREATABLE_TYPE_HELPER(TradeHistoryListModel);
    QML_REGISTER_UNCREATABLE_TYPE_HELPER(PaymentNodeStateModel);
    QML_REGISTER_UNCREATABLE_TYPE_HELPER(ConnextBrowserNodeApiTransport);
    QML_REGISTER_UNCREATABLE_TYPE_HELPER(TradingBotModel);
}

//==============================================================================

void QMLUtils::RegisterFonts()
{
    QFontDatabase::addApplicationFont("qrc:/Rubik-Black.tff");
    QFontDatabase::addApplicationFont("qrc:/Rubik-BlackItalic.tff");
    QFontDatabase::addApplicationFont("qrc:/Rubik-Bold.tff");
    QFontDatabase::addApplicationFont("qrc:/Rubik-BoldItalic.tff");
    QFontDatabase::addApplicationFont("qrc:/Rubik-Italic.tff");
    QFontDatabase::addApplicationFont("qrc:/Rubik-Light.tff");
    QFontDatabase::addApplicationFont("qrc:/Rubik-LightItalic.tff");
    QFontDatabase::addApplicationFont("qrc:/Rubik-Medium.tff");
    QFontDatabase::addApplicationFont("qrc:/Rubik-MediumItalic.tff");
    QFontDatabase::addApplicationFont("qrc:/Rubik-Regular.ttf");
    QFont font = QFont("Rubik");
}

//==============================================================================

void QMLUtils::RegisterUtils()
{
#undef uri
#define uri "com.xsn.utils"
    QML_REGISTER_UNCREATABLE_TYPE_HELPER(Enums);

    qmlRegisterSingletonType<QMLClipboardAdapter>(
        uri, 1, 0, "Clipboard", [](QQmlEngine* engine, QJSEngine*) -> QObject* {
            static QMLClipboardAdapter adapter;
            engine->setObjectOwnership(&adapter, QQmlEngine::CppOwnership);
            return &adapter;
        });

    qmlRegisterSingletonType<SkinColors>(
        uri, 1, 0, "SkinColors", [](QQmlEngine* engine, QJSEngine*) -> QObject* {
            engine->setObjectOwnership(SkinColors::Instance(), QQmlEngine::CppOwnership);
            return SkinColors::Instance();
        });

    static auto* utils = new UtilsGadget;
    qmlRegisterSingletonType<UtilsGadget>(
        uri, 1, 0, "Utils", [](QQmlEngine* engine, QJSEngine*) -> QObject* {
            engine->setObjectOwnership(utils, QQmlEngine::CppOwnership);
            return utils;
        });

    qmlRegisterSingletonType<AppUpdater>(
        uri, 1, 0, "AppUpdater", [](QQmlEngine* engine, QJSEngine*) -> QObject* {
            engine->setObjectOwnership(AppUpdater::Instance(), QQmlEngine::CppOwnership);
            return AppUpdater::Instance();
        });

    qmlRegisterSingletonType<TorManager>(
        uri, 1, 0, "TorManager", [](QQmlEngine* engine, QJSEngine*) -> QObject* {
            engine->setObjectOwnership(TorManager::Instance(), QQmlEngine::CppOwnership);
            return TorManager::Instance();
        });

    qmlRegisterSingletonType<MouseEventSpy>(
        uri, 1, 0, "MouseEventSpy", [](QQmlEngine* engine, QJSEngine*) -> QObject* {
            engine->setObjectOwnership(MouseEventSpy::Instance(), QQmlEngine::CppOwnership);
            return MouseEventSpy::Instance();
        });
}

//==============================================================================

UtilsGadget::UtilsGadget(QObject* parent)
    : QObject(parent)
{
}

//==============================================================================

UtilsGadget::~UtilsGadget() {}

//==============================================================================

QString UtilsGadget::formatBalance(QVariant balance, unsigned numberOfDecimals) const
{
    bool ok;
    {
        auto value = balance.toLongLong(&ok);
        if (ok) {
            return FormatAmount(value, numberOfDecimals);
        }
    }

    auto str = balance.toString();
    {
        auto value = str.toDouble(&ok);
        if (ok) {
            return FormatAmount(value, numberOfDecimals);
        }
    }

    {
        auto value = str.toLongLong(&ok);
        if (ok) {
            return FormatAmount(value, numberOfDecimals);
        }
    }

    return QString();
}

//==============================================================================

QString UtilsGadget::formatDate(QVariant msecsSinceEpoch) const
{
    bool ok;
    auto value = msecsSinceEpoch.toLongLong(&ok);
    return ok ? QDateTime::fromMSecsSinceEpoch(value).toString("yyyy-MM-dd HH:mm:ss") : QString();
}

//==============================================================================

QString UtilsGadget::mobileFormatDate(QVariant msecsSinceEpoch) const
{
    bool ok;
    auto value = msecsSinceEpoch.toLongLong(&ok);
    return ok ? QDateTime::fromMSecsSinceEpoch(value).toString("dd MMM yyyy") : QString();
}

//==============================================================================

QString UtilsGadget::formatTime(QVariant secsSinceEpoch, bool showFullTime) const
{
    bool ok;
    auto value = secsSinceEpoch.toLongLong(&ok);
    return ok && secsSinceEpoch > 0
        ? DateDifference(QDateTime::currentDateTime(), QDateTime::fromSecsSinceEpoch(value), false,
              true, showFullTime)
        : QString();
}

//==============================================================================

QString UtilsGadget::formatChannelsTime(QVariant mSecsSinceEpoch, bool showFullTime) const
{
    bool ok;
    auto value = mSecsSinceEpoch.toLongLong(&ok);

    return ok && mSecsSinceEpoch > 0 ? DateDifference(QDateTime::fromMSecsSinceEpoch(value),
                                           QDateTime::currentDateTime(), false, true, showFullTime)
                                     : QString();
}

//==============================================================================

QString UtilsGadget::formatDuration(int hoursSum) const
{
    auto days = static_cast<int>(std::floor(hoursSum / HOURS_PER_DAY));
    auto hours = hoursSum % HOURS_PER_DAY;

    auto hoursSymbol = QString("hour%1").arg(hours > 1 ? "s" : "");
    auto daysSymbol = QString("%1").arg(days > 1 ? "days" : (days == 1 ? "day" : ""));

    if (days >= 1) {
        if (hoursSum % HOURS_PER_DAY > 0) {
            return QString("%1 %2 %3 %4").arg(days).arg(daysSymbol).arg(hours).arg(hoursSymbol);
        } else {
            return QString("%1 %2").arg(days).arg(daysSymbol);
        }
    } else {
        return QString("%1 %2").arg(hours).arg(hoursSymbol);
    }
}

//==============================================================================

double UtilsGadget::convertSatoshiToCoin(double satoshi)
{
    return satoshi / COIN;
}

//==============================================================================

double UtilsGadget::convertCoinToSatoshi(double coins)
{
    return coins * COIN;
}

//==============================================================================

double UtilsGadget::parseCoinsToSatoshi(QString value)
{
    return ParseUserInputCoin(value);
}

//==============================================================================

QVariantList UtilsGadget::changeLog()
{
    QString changeLogPath = CHANGE_LOG_FILE_PATH;

    QString localFilePath = QUrl(changeLogPath).toLocalFile();
    bool isLocalFile = localFilePath.isEmpty();

    QString filePath = isLocalFile ? changeLogPath : QUrl(changeLogPath).toLocalFile();

    QFile file(filePath);

    if (file.open(QFile::ReadOnly)) {

        QString changeLog = QString::fromStdString(file.readAll().toStdString());
        QRegularExpression re("\\[.+\\]\\ -\\ \\d{4}-\\d{2}-\\d{2}");
        QRegularExpressionMatchIterator i = re.globalMatch(changeLog);

        QVariantList changeLogData;
        int lastIndex = -1;

        auto parseChangleLogData = [&changeLog, &changeLogData](int lastIndex, int currentIndex) {
            int newLineIndex = changeLog.indexOf('\n', lastIndex);

            auto startContentIndex = newLineIndex + 1;
            QString updateContent
                = changeLog.mid(startContentIndex, currentIndex - startContentIndex);
            QString verDate = changeLog.mid(lastIndex, newLineIndex - lastIndex);
            int firstSpaceIndex = verDate.lastIndexOf(' ');
            QString version = verDate.mid(1, verDate.indexOf(']') - 1);
            QString date = verDate.mid(firstSpaceIndex + 1, verDate.length() - firstSpaceIndex);

            QVariantMap versionContent;
            versionContent["version"] = version;
            versionContent["date"] = date;
            versionContent["updateContent"] = updateContent;
            changeLogData.append(versionContent);
        };

        while (i.hasNext()) {
            QRegularExpressionMatch match = i.next();
            auto currentIndex = match.capturedStart();
            QString versionDate = match.captured(0);

            if (lastIndex >= 0) {
                parseChangleLogData(lastIndex, currentIndex);
            }

            lastIndex = currentIndex;
        }

        parseChangleLogData(lastIndex, changeLog.lastIndexOf('\n'));

        return changeLogData;
    }

    return {};
}

//==============================================================================
