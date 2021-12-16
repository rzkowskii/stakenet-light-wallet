#ifndef QMLUTILS_HPP
#define QMLUTILS_HPP

#include <QQmlContext>

class UtilsGadget : public QObject {
    Q_OBJECT
public:
    explicit UtilsGadget(QObject* parent = nullptr);
    virtual ~UtilsGadget();

public slots:
    QString formatBalance(QVariant balance, unsigned numberOfDecimals = 8) const;
    QString formatDate(QVariant msecsSinceEpoch) const;
    QString mobileFormatDate(QVariant msecsSinceEpoch) const;
    QString formatTime(QVariant secsSinceEpoch, bool showFullTime = false) const;
    QString formatChannelsTime(QVariant mSecsSinceEpoch, bool showFullTime = false) const;
    QString formatDuration(int hoursSum) const;

    double convertSatoshiToCoin(double satoshi);
    double convertCoinToSatoshi(double coins);
    double parseCoinsToSatoshi(QString value);

    QVariantList changeLog();
};

struct QMLUtils {
public:
    static void RegisterQMLTypes();
    static void setContextProperties(QQmlContext* context);

    struct Sizes {
        int menuWidthSmallMode = 185;

        int windowWidthSmallMode = 1180;
        int windowWidthLargeMode = 1250;

        int assetsViewWidthSmallMode = 130;
        int assetsViewWidthLargeMode = 150;

        int headerViewHeightSmallMode = 240;
        int headerViewHeightMediumMode = 300;
        int headerViewHeightLargeMode = 350;

        int coinsSizeSmallMode = 60;
        int coinsSizeMediumMode = 75;
        int coinsSizeLargeMode = 100;

        int menuItemHeightSmallMode = 38;

        int closedTransactionHeight = 55;
        int openedTransactionHeight = 100;
    };

private:
    static void RegisterViewModels();
    static void RegisterModels();
    static void RegisterFonts();
    static void RegisterUtils();
};

#endif // QMLUTILS_HPP
