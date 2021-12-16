#ifndef CURRENCYMODEL_HPP
#define CURRENCYMODEL_HPP

#include "Data/LocalCurrency.hpp"
#include <QAbstractListModel>
#include <QPointer>

class CurrencyModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles { CodeRole, NameRole, SymbolRole };
    explicit CurrencyModel(QObject* parent = nullptr);
    virtual ~CurrencyModel() override;

    virtual int rowCount(const QModelIndex& parent) const override final;
    virtual QVariant data(const QModelIndex& index, int role) const override final;
    virtual QHash<int, QByteArray> roleNames() const override final;

public slots:
    void initialize(QObject* appViewModel);
    QString getCode(int index);
    int getInitial(QString code);

private:
    void initCurrency(LocalCurrency* localCurrency);

private:
    LocalCurrency::CurrencyList _currencies;
    QPointer<LocalCurrency> _localCurrency;
};
#endif // CURRENCYMODEL_HPP
