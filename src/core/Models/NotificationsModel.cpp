#include "NotificationsModel.hpp"
#include <Data/NotificationData.hpp>
#include <ViewModels/ApplicationViewModel.hpp>

#include <QDateTime>
#include <Tools/Common.hpp>

//==============================================================================

NotificationsModel::NotificationsModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

//==============================================================================

NotificationsModel::~NotificationsModel() {}

//==============================================================================

int NotificationsModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return static_cast<int>(_rowCount);
}

//==============================================================================

QVariant NotificationsModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount(QModelIndex())) {
        return QVariant();
    }

    const auto notification
        = _notificationData->notificationsList().at(static_cast<size_t>(index.row()));

    switch (role) {
    case TypeRole:
        return static_cast<int>(notification.type);
    case MessageRole:
        return notification.message;
    case ExistenceTimeRole:
        return DateDifference(notification.creationTime, QDateTime::currentDateTime(), true)
            .toLower();
    default:
        break;
    }

    return QVariant();
}

//==============================================================================

QHash<int, QByteArray> NotificationsModel::roleNames() const
{
    static QHash<int, QByteArray> roles;

    if (roles.empty()) {
        roles[TypeRole] = "type";
        roles[MessageRole] = "message";
        roles[ExistenceTimeRole] = "existenceTime";
    }

    return roles;
}

//==============================================================================

void NotificationsModel::initialize(QObject* appViewModel)
{
    if (auto viewModel = qobject_cast<ApplicationViewModel*>(appViewModel)) {
        beginResetModel();
        _notificationData = viewModel->notificationData();
        _rowCount = _notificationData->notificationsList().size();
        endResetModel();
    }
    connect(_notificationData, &NotificationData::notificationAdded, this,
        &NotificationsModel::onNotificationAdded);
}

//==============================================================================

void NotificationsModel::removeNotification(unsigned index) {}

//==============================================================================

void NotificationsModel::onNotificationAdded(Notification notification)
{
    Q_UNUSED(notification);
    int rows = rowCount(QModelIndex());
    beginInsertRows(QModelIndex(), rows, rows);
    ++_rowCount;
    endInsertRows();
}

//==============================================================================
