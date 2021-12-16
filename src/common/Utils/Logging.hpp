#ifndef LOGGING_HPP
#define LOGGING_HPP

#include <QDebug>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(Sync)
Q_DECLARE_LOGGING_CATEGORY(Api)
Q_DECLARE_LOGGING_CATEGORY(Chains)
Q_DECLARE_LOGGING_CATEGORY(Lnd)
Q_DECLARE_LOGGING_CATEGORY(Connext)
Q_DECLARE_LOGGING_CATEGORY(GRPC)
Q_DECLARE_LOGGING_CATEGORY(General)
Q_DECLARE_LOGGING_CATEGORY(WalletBackend)
Q_DECLARE_LOGGING_CATEGORY(Swaps)
Q_DECLARE_LOGGING_CATEGORY(Orderbook)
Q_DECLARE_LOGGING_CATEGORY(Web3)
Q_DECLARE_LOGGING_CATEGORY(TradingBot)

#define LogDebug() qDebug() << __FUNCTION__ << "|"
#define LogCritical() qCritical() << __FUNCTION__ << "|"
#define LogWarning() qWarning() << __FUNCTION__ << "|"
#define LogInfo() qInfo() << __FUNCTION__ << "|"
#define LogCDebug(Category) qCDebug(Category) << __FUNCTION__ << "|"
#define LogCCritical(Category) qCCritical(Category) << __FUNCTION__ << "|"
#define LogCWarning(Category) qCWarning(Category) << __FUNCTION__ << "|"
#define LogCCInfo(Category) qCInfo(Category) << __FUNCTION__ << "|"

#endif // LOGGING_HPP
