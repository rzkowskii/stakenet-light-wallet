#ifndef NOTIFICATIONDATA_HPP
#define NOTIFICATIONDATA_HPP

#include <Data/AbstractOrderbookDataSource.hpp>
#include <QDateTime>
#include <QObject>
#include <Tools/Common.hpp>

class WalletAssetsModel;

struct Notification {
    Enums::NotificationType type;
    QString message;
    QDateTime creationTime;
};

using NotificationsList = std::vector<Notification>;

class NotificationData : public QObject {
    Q_OBJECT
public:
    explicit NotificationData(const WalletAssetsModel& assetsModel, QObject* parent = nullptr);
    virtual ~NotificationData();

    const NotificationsList& notificationsList() const;
    void removeNotification(unsigned index);

signals:
    void orderPlaced(OrderSummary order);
    void swapCompleted(OrderSummary order);
    void depositCompleted(Balance balance, QString symbol); // deposit type
    void withdrawFinished(Balance balance, QString symbol); // withdraw type

    void notificationAdded(Notification notification);

private slots:
    void onSwapCompleted(OrderSummary order);
    void onOrderPlaced(OrderSummary order);
    void onDepositCompleted(Balance balance, QString symbol);
    void onWithdrawFinished(Balance balance, QString symbol);

private:
    void init(); // temporary for testing

private:
    const WalletAssetsModel& _assetsModel;
    NotificationsList _notificationsList;
};

#endif // NOTIFICATIONDATA_HPP
