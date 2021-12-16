#ifndef SKINCOLORS_HPP
#define SKINCOLORS_HPP

#include <QFileSystemWatcher>
#include <QJsonObject>
#include <QObject>
#include <QtGui/QColor>

class SkinColors : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString mainBackgroundPic READ mainBackgroundPic NOTIFY colorChanged)
    Q_PROPERTY(QString mainWalletLogo READ mainWalletLogo NOTIFY colorChanged)
    Q_PROPERTY(QString walletSideMenuLogo READ walletSideMenuLogo NOTIFY colorChanged)
    Q_PROPERTY(QString skinName READ skinName NOTIFY colorChanged)

    Q_PROPERTY(QColor mainBackground READ mainBackground NOTIFY colorChanged)
    Q_PROPERTY(QColor secondaryBackground READ secondaryBackground NOTIFY colorChanged)
    Q_PROPERTY(QColor mainText READ mainText NOTIFY colorChanged)
    Q_PROPERTY(QColor secondaryText READ secondaryText NOTIFY colorChanged)
    Q_PROPERTY(QColor menuBackground READ menuBackground NOTIFY colorChanged)
    Q_PROPERTY(QColor assetsMenuBackground READ assetsMenuBackground NOTIFY colorChanged)
    Q_PROPERTY(QColor menuItemSelectedText READ menuItemSelectedText NOTIFY colorChanged)
    Q_PROPERTY(QColor menuItemText READ menuItemText NOTIFY colorChanged)
    Q_PROPERTY(QColor menuItemContainsMouseText READ menuItemContainsMouseText NOTIFY colorChanged)
    Q_PROPERTY(
        QColor menuItemSelectedBackground READ menuItemSelectedBackground NOTIFY colorChanged)
    Q_PROPERTY(QColor headerText READ headerText NOTIFY colorChanged)
    Q_PROPERTY(QColor headerBackground READ headerBackground NOTIFY colorChanged)
    Q_PROPERTY(QColor slideBarBackground READ slideBarBackground NOTIFY colorChanged)
    Q_PROPERTY(QColor buttonsIcon READ buttonsIcon NOTIFY colorChanged)
    Q_PROPERTY(QColor transactionItemSelectedBackground READ transactionItemSelectedBackground
            NOTIFY colorChanged)
    Q_PROPERTY(
        QColor transactionItemSecondaryText READ transactionItemSecondaryText NOTIFY colorChanged)
    Q_PROPERTY(QColor transactionItemSent READ transactionItemSent NOTIFY colorChanged)
    Q_PROPERTY(QColor transactionItemReceived READ transactionItemReceived NOTIFY colorChanged)

    Q_PROPERTY(QColor walletAssetsBackgroundGradient READ walletAssetsBackgroundGradient NOTIFY
            colorChanged)
    Q_PROPERTY(QColor walletButtonsBackgroundGradient READ walletButtonsBackgroundGradient NOTIFY
            colorChanged)
    Q_PROPERTY(QColor comboboxIndicatorColor READ comboboxIndicatorColor NOTIFY colorChanged)

    Q_PROPERTY(
        QColor settingsPageHeaderGradient READ settingsPageHeaderGradient NOTIFY colorChanged)
    Q_PROPERTY(QColor settingPageHeaderText READ settingPageHeaderText NOTIFY colorChanged)
    Q_PROPERTY(
        QColor settingPageHeaderCurrentText READ settingPageHeaderCurrentText NOTIFY colorChanged)
    Q_PROPERTY(QColor settingsAssetsBackground READ settingsAssetsBackground NOTIFY colorChanged)
    Q_PROPERTY(QColor settingsActiveAsset READ settingsActiveAsset NOTIFY colorChanged)
    Q_PROPERTY(QColor settingsShareIcons READ settingsShareIcons NOTIFY colorChanged)
    Q_PROPERTY(QColor settingPagePersonalTabTextArea READ settingPagePersonalTabTextArea NOTIFY
            colorChanged)
    Q_PROPERTY(
        QColor assetsRoundedCheckboxBorder READ assetsRoundedCheckboxBorder NOTIFY colorChanged)

    Q_PROPERTY(QColor lightningPageInfoText READ lightningPageInfoText NOTIFY colorChanged)
    Q_PROPERTY(QColor lightningPageWarningGradientFirst READ lightningPageWarningGradientFirst
            NOTIFY colorChanged)
    Q_PROPERTY(QColor lightningPageWarningGradientSecond READ lightningPageWarningGradientSecond
            NOTIFY colorChanged)

    Q_PROPERTY(
        QColor menuBackgroundGradientFirst READ menuBackgroundGradientFirst NOTIFY colorChanged)
    Q_PROPERTY(
        QColor menuBackgroundGradientSecond READ menuBackgroundGradientSecond NOTIFY colorChanged)
    Q_PROPERTY(
        QColor menuBackgroundGradientThird READ menuBackgroundGradientThird NOTIFY colorChanged)
    Q_PROPERTY(QColor menuBackgroundGradienRightLine READ menuBackgroundGradienRightLine NOTIFY
            colorChanged)

    Q_PROPERTY(QColor popupFieldBorder READ popupFieldBorder NOTIFY colorChanged)
    Q_PROPERTY(QColor popupInfoText READ popupInfoText NOTIFY colorChanged)
    Q_PROPERTY(QColor sendPopupConfirmText READ sendPopupConfirmText NOTIFY colorChanged)

    Q_PROPERTY(QColor highlightedItemHeader READ highlightedItemHeader NOTIFY colorChanged)
    Q_PROPERTY(QColor highlightedMenuItem READ highlightedMenuItem NOTIFY colorChanged)
    Q_PROPERTY(QColor highlightedAssetsItem READ highlightedAssetsItem NOTIFY colorChanged)

    Q_PROPERTY(QColor walletButtonsHoveredBackgroundGradient READ
            walletButtonsHoveredBackgroundGradient NOTIFY colorChanged)
    Q_PROPERTY(QColor menuBackgroundHoveredGradienRightLine READ
            menuBackgroundHoveredGradienRightLine NOTIFY colorChanged)

    Q_PROPERTY(QColor magnifyingGlass READ magnifyingGlass NOTIFY colorChanged)
    Q_PROPERTY(QColor errorViewBackground READ errorViewBackground NOTIFY colorChanged)
    Q_PROPERTY(QColor orderBookEvenBackground READ orderBookEvenBackground NOTIFY colorChanged)

    Q_PROPERTY(QColor introBtnHoveredColor READ introBtnHoveredColor NOTIFY colorChanged)
    Q_PROPERTY(QColor introBtnColor READ introBtnColor NOTIFY colorChanged)
    Q_PROPERTY(
        QColor introBtnGradientHoveredColor READ introBtnGradientHoveredColor NOTIFY colorChanged)
    Q_PROPERTY(QColor introBtnGradientColor READ introBtnGradientColor NOTIFY colorChanged)
    Q_PROPERTY(QColor popupsBgColor READ popupsBgColor NOTIFY colorChanged)
    Q_PROPERTY(
        QColor switchActiveIndicatorColor READ switchActiveIndicatorColor NOTIFY colorChanged)
    Q_PROPERTY(QColor switchActiveIndicatorBorderColor READ switchActiveIndicatorBorderColor NOTIFY
            colorChanged)
    Q_PROPERTY(
        QColor notificationViewBackground READ notificationViewBackground NOTIFY colorChanged)

    Q_PROPERTY(QColor activeNotificationItem READ activeNotificationItem NOTIFY colorChanged)
    Q_PROPERTY(QColor buttonBorderColor READ buttonBorderColor NOTIFY colorChanged)
    Q_PROPERTY(QColor dexDisclaimerTextColor READ dexDisclaimerTextColor NOTIFY colorChanged)

    Q_PROPERTY(
        QColor cancelButtonHoveredBgrColor READ cancelButtonHoveredBgrColor NOTIFY colorChanged)
    Q_PROPERTY(QColor cancelButtonColor READ cancelButtonColor NOTIFY colorChanged)
    Q_PROPERTY(QColor cancelButtonGradientHoveredColor READ cancelButtonGradientHoveredColor NOTIFY
            colorChanged)
    Q_PROPERTY(QColor cancelButtonGradienColor READ cancelButtonGradienColor NOTIFY colorChanged)

    Q_PROPERTY(QColor swapComboBoxBackground READ swapComboBoxBackground NOTIFY colorChanged)

    Q_PROPERTY(
        QColor swapPageBackgroundHeaderFirst READ swapPageBackgroundHeaderFirst NOTIFY colorChanged)
    Q_PROPERTY(QColor swapPageBackgroundHeaderSecond READ swapPageBackgroundHeaderSecond NOTIFY
            colorChanged)
    Q_PROPERTY(
        QColor swapPageBackgroundHeaderThird READ swapPageBackgroundHeaderThird NOTIFY colorChanged)

    Q_PROPERTY(
        QColor swapPageBackgroundFooterFirst READ swapPageBackgroundFooterFirst NOTIFY colorChanged)
    Q_PROPERTY(QColor swapPageBackgroundFooterSecond READ swapPageBackgroundFooterSecond NOTIFY
            colorChanged)
    Q_PROPERTY(
        QColor swapPageBackgroundFooterThird READ swapPageBackgroundFooterThird NOTIFY colorChanged)

    Q_PROPERTY(
        QColor swapPageSecondaryBackground READ swapPageSecondaryBackground NOTIFY colorChanged)

    Q_PROPERTY(QColor botPageBackground READ botPageBackground NOTIFY colorChanged)
    Q_PROPERTY(QColor botPageBackgroundImageColorFirst READ botPageBackgroundImageColorFirst NOTIFY
            colorChanged)
    Q_PROPERTY(QColor botPageBackgroundImageColorSecond READ botPageBackgroundImageColorSecond
            NOTIFY colorChanged)
    Q_PROPERTY(QColor botPageBackgroundImageColorThird READ botPageBackgroundImageColorThird NOTIFY
            colorChanged)
    Q_PROPERTY(QColor botPageMainBackgroundColorFirst READ botPageMainBackgroundColorFirst NOTIFY
            colorChanged)
    Q_PROPERTY(QColor botPageMainBackgroundColorSecond READ botPageMainBackgroundColorSecond NOTIFY
            colorChanged)
    Q_PROPERTY(QColor botPageMainBackgroundColorThird READ botPageMainBackgroundColorThird NOTIFY
            colorChanged)

    Q_PROPERTY(
        QColor botDepthCurveImageBackground READ botDepthCurveImageBackground NOTIFY colorChanged)
    Q_PROPERTY(QColor botAssetImageBackground READ botAssetImageBackground NOTIFY colorChanged)

    Q_PROPERTY(
        QColor botTextFieldActiveStateColor READ botTextFieldActiveStateColor NOTIFY colorChanged)
    Q_PROPERTY(QColor botTextFieldInactiveStateColor READ botTextFieldInactiveStateColor NOTIFY
            colorChanged)
    Q_PROPERTY(
        QColor botTextFieldActiveBorderColor READ botTextFieldActiveBorderColor NOTIFY colorChanged)
    Q_PROPERTY(QColor botTextFieldInactiveBorderColor READ botTextFieldInactiveBorderColor NOTIFY
            colorChanged)

    Q_PROPERTY(
        QColor botComboBoxHighlightedColor READ botComboBoxHighlightedColor NOTIFY colorChanged)
    Q_PROPERTY(
        QColor botComboBoxBackgroundColor READ botComboBoxBackgroundColor NOTIFY colorChanged)
    Q_PROPERTY(QColor botComboBoxPopupBackgroundColor READ botComboBoxPopupBackgroundColor NOTIFY
            colorChanged)
    Q_PROPERTY(QColor botComboBoxPopupBackgroundBorderColor READ
            botComboBoxPopupBackgroundBorderColor NOTIFY colorChanged)

    Q_PROPERTY(QColor botSliderBackground READ botSliderBackground NOTIFY colorChanged)
    Q_PROPERTY(
        QColor botSliderBackgroundColorFirst READ botSliderBackgroundColorFirst NOTIFY colorChanged)
    Q_PROPERTY(QColor botSliderBackgroundColorSecond READ botSliderBackgroundColorSecond NOTIFY
            colorChanged)
    Q_PROPERTY(
        QColor botSliderHandlePressedColor READ botSliderHandlePressedColor NOTIFY colorChanged)
    Q_PROPERTY(
        QColor botSliderHandleInactiveColor READ botSliderHandleInactiveColor NOTIFY colorChanged)
    Q_PROPERTY(
        QColor botSliderHandleBorderColor READ botSliderHandleBorderColor NOTIFY colorChanged)
    Q_PROPERTY(
        QColor botSliderHandleLines READ botSliderHandleLines NOTIFY colorChanged)

    Q_PROPERTY(QColor mobileButtonBackground READ mobileButtonBackground NOTIFY colorChanged)
    Q_PROPERTY(QColor mobileWalletReceive READ mobileWalletReceive NOTIFY colorChanged)
    Q_PROPERTY(QColor mobileLineSeparator READ mobileLineSeparator NOTIFY colorChanged)
    Q_PROPERTY(
        QColor mobileTopGradientBackground READ mobileTopGradientBackground NOTIFY colorChanged)
    Q_PROPERTY(QColor mobileMiddleGradientBackground READ mobileMiddleGradientBackground NOTIFY
            colorChanged)
    Q_PROPERTY(QColor mobileBottomGradientBackground READ mobileBottomGradientBackground NOTIFY
            colorChanged)
    Q_PROPERTY(QColor mobileTitle READ mobileTitle NOTIFY colorChanged)
    Q_PROPERTY(QColor mobileSecondaryBackground READ mobileSecondaryBackground NOTIFY colorChanged)
    Q_PROPERTY(
        QColor mobileIconButtonBackground READ mobileIconButtonBackground NOTIFY colorChanged)
    Q_PROPERTY(
        QColor mobileActiveSwitchBackground READ mobileActiveSwitchBackground NOTIFY colorChanged)
    Q_PROPERTY(QColor mobileDisactiveSwitchBackground READ mobileDisactiveSwitchBackground NOTIFY
            colorChanged)
    Q_PROPERTY(
        QColor mobileActiveSwitchIndicator READ mobileActiveSwitchIndicator NOTIFY colorChanged)
    Q_PROPERTY(QColor mobileDisactiveSwitchIndicator READ mobileDisactiveSwitchIndicator NOTIFY
            colorChanged)
    Q_PROPERTY(QColor mobileLocalizationSelectedItemBackground READ
            mobileLocalizationSelectedItemBackground NOTIFY colorChanged)
    Q_PROPERTY(QColor mobileSettingsSecondaryBackground READ mobileSettingsSecondaryBackground
            NOTIFY colorChanged)
    Q_PROPERTY(
        QColor mobileWalletItemBackground READ mobileWalletItemBackground NOTIFY colorChanged)
    Q_PROPERTY(QColor mobileRestoreAdviceText READ mobileRestoreAdviceText NOTIFY colorChanged)
    Q_PROPERTY(QColor walletPageBackgroundLightColor READ walletPageBackgroundLightColor NOTIFY colorChanged)
    Q_PROPERTY(QColor walletPageHeaderViewBlueColor READ walletPageHeaderViewBlueColor NOTIFY colorChanged)
    Q_PROPERTY(QColor delegatesBackgroundLightColor READ delegatesBackgroundLightColor NOTIFY colorChanged)
    Q_PROPERTY(QColor delegatesBackgroundDarkColor READ delegatesBackgroundDarkColor NOTIFY colorChanged)
    Q_PROPERTY(QColor tabDelegateLightColor READ tabDelegateLightColor NOTIFY colorChanged)
    Q_PROPERTY(QColor tabDelegateDarkColor READ tabDelegateDarkColor NOTIFY colorChanged)
    Q_PROPERTY(QColor walletAssetHighlightColor READ walletAssetHighlightColor NOTIFY colorChanged)


    Q_PROPERTY(QColor dexPageBackground READ dexPageBackground NOTIFY colorChanged)
    Q_PROPERTY(QColor mainDexViewBackgroundGradient1 READ mainDexViewBackgroundGradient1 NOTIFY colorChanged)
    Q_PROPERTY(QColor mainDexViewBackgroundGradient2 READ mainDexViewBackgroundGradient2 NOTIFY colorChanged)
    Q_PROPERTY(QColor dexPageSecondaryBackground READ dexPageSecondaryBackground NOTIFY colorChanged)
    Q_PROPERTY(QColor dexBalancesViewBackground READ dexBalancesViewBackground NOTIFY colorChanged)
    Q_PROPERTY(QColor dexPercentViewSeparator READ dexPercentViewSeparator NOTIFY colorChanged)
    Q_PROPERTY(QColor dexSubHeaderBackgroundGradient1 READ dexSubHeaderBackgroundGradient1 NOTIFY colorChanged)
    Q_PROPERTY(QColor dexSubHeaderBackgroundGradient2 READ dexSubHeaderBackgroundGradient2 NOTIFY colorChanged)

public:
    static SkinColors* Instance();

    QString skinName() const;

    QString mainBackgroundPic() const;
    QString mainWalletLogo() const;
    QString walletSideMenuLogo() const;
    QColor mainBackground() const;
    QColor secondaryBackground() const;
    QColor mainText() const;
    QColor secondaryText() const;
    QColor menuBackground() const;
    QColor menuItemSelectedText() const;
    QColor menuItemText() const;
    QColor menuItemContainsMouseText() const;
    QColor menuItemSelectedBackground() const;
    QColor headerText() const;
    QColor headerBackground() const;
    QColor slideBarBackground() const;
    QColor buttonsIcon() const;
    QColor transactionItemSelectedBackground() const;
    QColor transactionItemSecondaryText() const;
    QColor transactionItemSent() const;
    QColor transactionItemReceived() const;

    QColor walletAssetsBackgroundGradient() const;
    QColor walletButtonsBackgroundGradient() const;
    QColor comboboxIndicatorColor() const;

    QColor settingsPageHeaderGradient() const;
    QColor settingPageHeaderText() const;
    QColor settingPageHeaderCurrentText() const;
    QColor settingPagePersonalTabTextArea() const;
    QColor settingsAssetsBackground() const;
    QColor settingsActiveAsset() const;
    QColor settingsShareIcons() const;
    QColor assetsRoundedCheckboxBorder() const;
    QColor assetsMenuBackground() const;

    QColor lightningPageInfoText() const;
    QColor lightningPageWarningGradientFirst() const;
    QColor lightningPageWarningGradientSecond() const;

    QColor menuBackgroundGradientFirst() const;
    QColor menuBackgroundGradientSecond() const;
    QColor menuBackgroundGradientThird() const;
    QColor menuBackgroundGradienRightLine() const;

    QColor popupFieldBorder() const;
    QColor popupInfoText() const;
    QColor sendPopupConfirmText() const;

    QColor highlightedItemHeader() const;
    QColor highlightedMenuItem() const;
    QColor highlightedAssetsItem() const;

    QColor walletButtonsHoveredBackgroundGradient() const;
    QColor menuBackgroundHoveredGradienRightLine() const;

    QColor errorViewBackground() const;
    QColor orderBookEvenBackground() const;

    QColor introBtnHoveredColor() const;
    QColor introBtnColor() const;
    QColor introBtnGradientHoveredColor() const;
    QColor introBtnGradientColor() const;
    QColor popupsBgColor() const;

    QColor switchActiveIndicatorColor() const;
    QColor switchActiveIndicatorBorderColor() const;

    QColor notificationViewBackground() const;
    QColor activeNotificationItem() const;
    QColor buttonBorderColor() const;
    QColor dexDisclaimerTextColor() const;

    QColor cancelButtonHoveredBgrColor() const;
    QColor cancelButtonColor() const;
    QColor cancelButtonGradientHoveredColor() const;
    QColor cancelButtonGradienColor() const;

    QColor swapComboBoxBackground() const;

    QColor swapPageBackgroundHeaderFirst() const;
    QColor swapPageBackgroundHeaderSecond() const;
    QColor swapPageBackgroundHeaderThird() const;

    QColor swapPageBackgroundFooterFirst() const;
    QColor swapPageBackgroundFooterSecond() const;
    QColor swapPageBackgroundFooterThird() const;

    QColor swapPageSecondaryBackground() const;

    QColor botPageBackground() const;
    QColor botPageBackgroundImageColorFirst() const;
    QColor botPageBackgroundImageColorSecond() const;
    QColor botPageBackgroundImageColorThird() const;
    QColor botPageMainBackgroundColorFirst() const;
    QColor botPageMainBackgroundColorSecond() const;
    QColor botPageMainBackgroundColorThird() const;

    QColor botDepthCurveImageBackground() const;
    QColor botAssetImageBackground() const;

    QColor botTextFieldActiveStateColor() const;
    QColor botTextFieldInactiveStateColor() const;
    QColor botTextFieldActiveBorderColor() const;
    QColor botTextFieldInactiveBorderColor() const;

    QColor botComboBoxHighlightedColor() const;
    QColor botComboBoxBackgroundColor() const;
    QColor botComboBoxPopupBackgroundColor() const;
    QColor botComboBoxPopupBackgroundBorderColor() const;

    QColor botSliderBackground() const;
    QColor botSliderBackgroundColorFirst() const;
    QColor botSliderBackgroundColorSecond() const;
    QColor botSliderHandlePressedColor() const;
    QColor botSliderHandleInactiveColor() const;
    QColor botSliderHandleBorderColor() const;
    QColor botSliderHandleLines() const;

    QColor magnifyingGlass() const;
    QColor mobileButtonBackground() const;
    QColor mobileWalletReceive() const;
    QColor mobileLineSeparator() const;

    QColor mobileTopGradientBackground() const;
    QColor mobileMiddleGradientBackground() const;
    QColor mobileBottomGradientBackground() const;

    QColor mobileTitle() const;

    QColor mobileSecondaryBackground() const;
    QColor mobileIconButtonBackground() const;

    QColor mobileActiveSwitchBackground() const;
    QColor mobileDisactiveSwitchBackground() const;
    QColor mobileActiveSwitchIndicator() const;
    QColor mobileDisactiveSwitchIndicator() const;

    QColor mobileLocalizationSelectedItemBackground() const;
    QColor mobileSettingsSecondaryBackground() const;
    QColor mobileWalletItemBackground() const;
    QColor mobileRestoreAdviceText() const;
    QColor walletPageBackgroundLightColor() const;
    QColor walletPageHeaderViewBlueColor() const;
    QColor delegatesBackgroundLightColor() const;
    QColor delegatesBackgroundDarkColor() const;
    QColor tabDelegateLightColor() const;
    QColor tabDelegateDarkColor() const;
    QColor walletAssetHighlightColor() const;

    QColor dexPageBackground() const;
    QColor mainDexViewBackgroundGradient1() const;
    QColor mainDexViewBackgroundGradient2() const;
    QColor dexPageSecondaryBackground() const;
    QColor dexBalancesViewBackground() const;
    QColor dexPercentViewSeparator() const;
    QColor dexSubHeaderBackgroundGradient1() const;
    QColor dexSubHeaderBackgroundGradient2() const;


signals:
    void colorChanged();

public slots:
    void initSkin(QString skinFilePath, QString skinName);
    bool isActiveSkin(QString skinFilePath);

private slots:
    void initCustomSkin(QString skinFilePath);

private:
    QColor getColor(QString name) const;

    void writeSettings() const;
    void readSettings();

private:
    explicit SkinColors(QObject* parent = nullptr);
    QJsonObject _skinColors;
    QString _skinFilePath;
    QString _skinName;
    QFileSystemWatcher* _fileSystemWatcher = nullptr;
};

#endif // SKINCOLORS_HPP
