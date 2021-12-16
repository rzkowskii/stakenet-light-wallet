#ifndef SORTFILTERWALLETASSETSLISTMODEL_HPP
#define SORTFILTERWALLETASSETSLISTMODEL_HPP

#include <QObject>
#include <QSortFilterProxyModel>

class QMLSortFilterListProxyModel : public QSortFilterProxyModel {
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QObject* source READ source WRITE setSource)
    Q_PROPERTY(QByteArray filterRole READ filterRole WRITE setFilterRole)
    Q_PROPERTY(QString filterString READ filterString WRITE setFilterString)
    Q_PROPERTY(QByteArray sortRole READ sortRole WRITE setSortRole)
    Q_PROPERTY(bool sortAsc READ sortAsc WRITE setSortAsc)

public:
    explicit QMLSortFilterListProxyModel(QObject* parent = nullptr);

    QObject* source() const;
    void setSource(QObject* source);

    QByteArray filterRole() const;
    void setFilterRole(const QByteArray& role);

    QString filterString() const;
    void setFilterString(const QString& filter);

    QByteArray sortRole() const;
    void setSortRole(const QByteArray& role);

    bool sortAsc() const;
    void setSortAsc(bool value);

    int count() const;

signals:
    void countChanged();

public slots:
    QVariantMap get(int row);

protected:
    int roleKey(const QByteArray& role) const;
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;

private:
    QByteArray _sortRole;
    QByteArray _filterRole;
};

#endif // SORTFILTERWALLETASSETSLISTMODEL_HPP
