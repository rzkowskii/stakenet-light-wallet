#include "NotificationData.hpp"
#include <Data/SkinColors.hpp>
#include <Data/WalletAssetsModel.hpp>

//==============================================================================

NotificationData::NotificationData(const WalletAssetsModel& assetsModel, QObject* parent)
    : QObject(parent)
    , _assetsModel(assetsModel)

{
    connect(this, &NotificationData::swapCompleted, this, &NotificationData::onSwapCompleted);
    connect(this, &NotificationData::orderPlaced, this, &NotificationData::onOrderPlaced);
    connect(this, &NotificationData::depositCompleted, this, &NotificationData::onDepositCompleted);
    connect(this, &NotificationData::withdrawFinished, this, &NotificationData::onWithdrawFinished);

    init();
}

//==============================================================================

NotificationData::~NotificationData() {}

//==============================================================================

const NotificationsList& NotificationData::notificationsList() const
{
    return _notificationsList;
}

//==============================================================================

void NotificationData::removeNotification(unsigned index) {}

//==============================================================================

void NotificationData::onSwapCompleted(OrderSummary order)
{
    Notification notification;
    notification.type = Enums::NotificationType::Swap;
    //    notification.message = QString("%1<font color=\"%2\"> %3</font> <font color=\"%4\">
    //    /%5</font> %6 %7 <font color=\"%4\"> @</font> %8")
    //                               .arg(order.pairId.toUpper())
    //                               .arg("white")
    //                               .arg(FormatAmount(order.amount))
    //                               .arg("green")
    //                               .arg(FormatAmount(order.price))
    //                               .arg(order.type == Enums::OrderType::Limit ? "Limit" :
    //                               "Market") .arg(order.side == Enums::OrderSide::Buy ? "Buy" :
    //                               "Sell") .arg(FormatAmount(order.price));
    notification.creationTime = QDateTime::currentDateTime();

    notificationAdded(notification);
}

//==============================================================================

void NotificationData::onOrderPlaced(OrderSummary order)
{
    Notification notification;
    //    auto isBuy = order.side == Enums::OrderSide::Buy;
    //    notification.type = isBuy ? Enums::NotificationType::BuyOrderPlaced :
    //    Enums::NotificationType::SellOrderPlaced; notification.message = QString("%1<font
    //    color=\"%2\"> %3</font> <font color=\"%4\"> /%5</font> %6 %7 <font color=\"%4\"> @</font>
    //    %8")
    //                               .arg(order.pairId.toUpper())
    //                               .arg("white")
    //                               .arg(FormatAmount(order.amount))
    //                               .arg("green")
    //                               .arg(FormatAmount(order.price))
    //                               .arg(order.type == Enums::OrderType::Limit ? "Limit" :
    //                               "Market") .arg(isBuy ? "Buy" : "Sell")
    //                               .arg(FormatAmount(order.price));
    notification.creationTime = QDateTime::currentDateTime();

    notificationAdded(notification);
}

//==============================================================================

void NotificationData::onDepositCompleted(Balance balance, QString symbol)
{
    Notification notification;
    notification.type = Enums::NotificationType::Deposit;
    notification.message = QString(
        "Deposit of <font color=\"%1\"> %2 %3</font>\n successfully deposited into your\n account.")
                               .arg("white")
                               .arg(FormatAmount(balance))
                               .arg(symbol.toUpper());
    notification.creationTime = QDateTime::currentDateTime();
}

//==============================================================================

void NotificationData::onWithdrawFinished(Balance balance, QString symbol)
{
    Notification notification;
    notification.type = Enums::NotificationType::Withdraw;
    notification.message = QString("Withdraw of <font color=\"%1\"> %2 %3</font> has been sent")
                               .arg("white")
                               .arg(FormatAmount(balance))
                               .arg(symbol.toUpper());
    notification.creationTime = QDateTime::currentDateTime();
}

//==============================================================================

void NotificationData::init()
{
    Notification notificationSwap;
    notificationSwap.type = Enums::NotificationType::Swap;
    notificationSwap.message = QString("%1/%2<font color=\"%3\"> %4</font> <font color=\"%5\"> "
                                       "/%6</font> %7 %8 <font color=\"%5\"> @</font> %9")
                                   .arg("LTC")
                                   .arg("XSN")
                                   .arg("white")
                                   .arg("0.04687936")
                                   .arg("green")
                                   .arg("364.93")
                                   .arg("Limit")
                                   .arg("Buy")
                                   .arg("7773.00");
    notificationSwap.creationTime = QDateTime::currentDateTime();

    Notification notificationBuyOrderPlaced;
    notificationBuyOrderPlaced.type = Enums::NotificationType::BuyOrderPlaced;
    notificationBuyOrderPlaced.message
        = QString("%1/%2<font color=\"%3\"> %4</font> <font color=\"%5\"> /%6</font> %7 %8 <font "
                  "color=\"%5\"> @</font> %9")
              .arg("LTC")
              .arg("XSN")
              .arg("white")
              .arg("0.04687936")
              .arg("green")
              .arg("364.93")
              .arg("Limit")
              .arg("Buy")
              .arg("7773.00");
    notificationBuyOrderPlaced.creationTime = QDateTime::fromSecsSinceEpoch(1583100302);

    Notification notificationDeposit;
    notificationDeposit.type = Enums::NotificationType::Deposit;
    notificationDeposit.message = QString(
        "Deposit of <font color=\"%1\"> %2 %3</font> successfully deposited into your account.")
                                      .arg("white")
                                      .arg("0.10007004")
                                      .arg("BTC");
    notificationDeposit.creationTime = QDateTime::fromSecsSinceEpoch(1583166202);

    Notification notificationWithdraw;
    notificationWithdraw.type = Enums::NotificationType::Withdraw;
    notificationWithdraw.message
        = QString("Withdraw of <font color=\"%1\"> %2 %3</font> has been sent")
              .arg("white")
              .arg("0.10006005")
              .arg("XSN");
    notificationWithdraw.creationTime = QDateTime::fromSecsSinceEpoch(1583166202);

    Notification notificationSellOrderPlaced;
    notificationSellOrderPlaced.type = Enums::NotificationType::SellOrderPlaced;
    notificationSellOrderPlaced.message
        = QString("%1/%2<font color=\"%3\"> %4</font> <font color=\"%5\"> /%6</font> %7 %8 <font "
                  "color=\"%5\"> @</font> %9")
              .arg("LTC")
              .arg("XSN")
              .arg("white")
              .arg("0.04687936")
              .arg("green")
              .arg("364.93")
              .arg("Limit")
              .arg("Buy")
              .arg("7773.00");
    notificationSellOrderPlaced.creationTime = QDateTime::fromSecsSinceEpoch(1583100402);

    _notificationsList.push_back(notificationSwap);
    _notificationsList.push_back(notificationBuyOrderPlaced);
    _notificationsList.push_back(notificationDeposit);
    _notificationsList.push_back(notificationWithdraw);
    _notificationsList.push_back(notificationSellOrderPlaced);
}

//==============================================================================
