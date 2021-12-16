#ifndef TRADINGBOTMODEL_HPP
#define TRADINGBOTMODEL_HPP

#include <QObject>

class TradingBotService;
class DexService;

class TradingBotModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isActive READ isActive NOTIFY activityChanged)

public:
    explicit TradingBotModel(const DexService& dexService, QObject* parent = nullptr);

    bool isActive() const;

signals:
    void activityChanged();

public slots:
    void start(int risk, int baseQuantity, int quoteQuantity);
    void stop();
    int gridLevels();

private:
    void setActivity(bool value);

private:
    const DexService& _dexService;
    bool _isActive {false};
};

#endif // TRADINGBOTMODEL_HPP
