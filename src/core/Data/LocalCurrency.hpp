#ifndef LOCALBALANCE_HPP
#define LOCALBALANCE_HPP

#include <QObject>
#include <QSettings>
#include <Tools/Common.hpp>
#include <tuple>
#include <vector>

using CurrencyCode = QString;
using CurrencySymbol = QString;
using CurrencyName = QString;

struct CurrencyEntry {
    CurrencyEntry() = default;
    CurrencyEntry(CurrencyCode code, CurrencyName name, CurrencySymbol symbol);

    CurrencyCode code;
    CurrencyName name;
    CurrencySymbol symbol;
};

class LocalCurrency : public QObject {
    Q_OBJECT
public:
    using CurrencyList = std::vector<CurrencyEntry>;
    explicit LocalCurrency(QObject* parent = nullptr);
    virtual ~LocalCurrency();

    CurrencyList currencies() const;
    CurrencyEntry activeCurrency() const;
    void setActiveCurrency(CurrencyCode code);

signals:
    void activeCurrencyChanged(CurrencyCode newActiveCurrency);

public slots:

private:
    void init();
    void addCurrency(CurrencyCode code, CurrencyName name, CurrencySymbol symbol);
    void writeSettings() const;
    void readSettings();

private:
    using CurrencyMap = std::map<CurrencyCode, CurrencyEntry>;
    CurrencyMap _currencies;
    CurrencyEntry _activeCurrency;
};

#endif // LOCALBALANCE_HPP
