#ifndef NOTIFICATIONSMODEL_HPP
#define NOTIFICATIONSMODEL_HPP

#include <QAbstractListModel>
#include <QPointer>
#include <Tools/Common.hpp>

class NotificationData;
struct Notification;

class NotificationsModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles { TypeRole, MessageRole, ExistenceTimeRole };

    explicit NotificationsModel(QObject* parent = nullptr);
    virtual ~NotificationsModel() override;

    virtual int rowCount(const QModelIndex& parent) const override final;
    virtual QVariant data(const QModelIndex& index, int role) const override final;
    virtual QHash<int, QByteArray> roleNames() const override final;

public slots:
    void initialize(QObject* appViewModel);
    void removeNotification(unsigned index);

private slots:
    void onNotificationAdded(Notification notification);

private:
    QPointer<NotificationData> _notificationData;
    size_t _rowCount{ 0 };
};

#endif // NOTIFICATIONSMODEL_HPP
