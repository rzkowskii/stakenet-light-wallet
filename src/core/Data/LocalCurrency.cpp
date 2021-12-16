#include "LocalCurrency.hpp"
#include <QVariant>

//==============================================================================

static const QString SETTINGS_ACTIVE_CURRENCY("activeCurrency");

//==============================================================================

LocalCurrency::LocalCurrency(QObject* parent)
    : QObject(parent)
{
    init();
}

//==============================================================================

LocalCurrency::~LocalCurrency() {}

//==============================================================================

LocalCurrency::CurrencyList LocalCurrency::currencies() const
{
    CurrencyList result;
    for (auto&& pair : _currencies) {
        result.push_back(pair.second);
    }

    return result;
}

//==============================================================================

CurrencyEntry LocalCurrency::activeCurrency() const
{
    return _activeCurrency;
}

//==============================================================================

void LocalCurrency::setActiveCurrency(CurrencyCode code)
{
    Q_ASSERT(_currencies.count(code) > 0);

    if (_activeCurrency.code != code) {
        _activeCurrency = _currencies.at(code);
        writeSettings();
        activeCurrencyChanged(_activeCurrency.code);
    }
}

//==============================================================================

void LocalCurrency::init()
{
    _currencies.emplace("JPY", CurrencyEntry("JPY", "Japanese Yen", "¥"));
    _currencies.emplace("MXN", CurrencyEntry("MXN", "Mexican peso", "$"));
    _currencies.emplace("NZD", CurrencyEntry("NZD", "New Zealand dollar", "NZ$"));
    _currencies.emplace("TRY", CurrencyEntry("TRY", "Turkish lira", "₺"));
    _currencies.emplace("USD", CurrencyEntry("USD", "United States dollar", "$"));
    _currencies.emplace("UAH", CurrencyEntry("UAH", "Ukraine hryvnia", "₴"));
    _currencies.emplace("EUR", CurrencyEntry("EUR", "Euro", "€"));
    _currencies.emplace("GBP", CurrencyEntry("GBP", "Pound sterling", "£"));

    readSettings();
}

//==============================================================================

void LocalCurrency::writeSettings() const
{
    QSettings settings;
    settings.setValue(SETTINGS_ACTIVE_CURRENCY, QVariant::fromValue(_activeCurrency.code));
    settings.sync();
}

//==============================================================================

void LocalCurrency::readSettings()
{
    QSettings settings;
    auto code = settings.value(SETTINGS_ACTIVE_CURRENCY).value<QString>();
    if (_currencies.count(code) > 0) {
        _activeCurrency = _currencies.at(code);
    } else {
        _activeCurrency = _currencies.at("USD"); // USD for default
    }
}

//==============================================================================

void LocalCurrency::addCurrency(QString code, QString name, QString symbol)
{
    _currencies.emplace(code, CurrencyEntry(code, name, symbol));
}

//==============================================================================

CurrencyEntry::CurrencyEntry(QString code, QString name, QString symbol)
    : code(code)
    , name(name)
    , symbol(symbol)
{
}

//==============================================================================
