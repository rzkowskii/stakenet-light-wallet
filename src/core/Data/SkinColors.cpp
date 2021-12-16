#include "SkinColors.hpp"
#include <Tools/Common.hpp>
#include <Utils/Logging.hpp>

#include <QFile>
#include <QFileSystemWatcher>
#include <QJsonDocument>
#include <QSettings>
#include <QUrl>

//==============================================================================

static const QString SETTINGS_ACTIVE_SKIN_PATH("activeSkinFile");
static const QString SETTINGS_ACTIVE_SKIN_NAME("activeSkinName");

//==============================================================================

SkinColors::SkinColors(QObject* parent)
    : QObject(parent)
{
    readSettings();
    _fileSystemWatcher = new QFileSystemWatcher(this);
    connect(
        _fileSystemWatcher, &QFileSystemWatcher::fileChanged, this, &SkinColors::initCustomSkin);

    initSkin(_skinFilePath, _skinName);
}

//==============================================================================

SkinColors* SkinColors::Instance()
{
    static SkinColors instance;
    return &instance;
}

//==============================================================================

QString SkinColors::skinName() const
{
    return _skinName;
}

//==============================================================================

QColor SkinColors::mainBackground() const
{
    return getColor("mainBackground");
}

//==============================================================================

QString SkinColors::mainBackgroundPic() const
{
    return _skinColors.value("mainBackgroundPic").toString();
}

//==============================================================================

QString SkinColors::mainWalletLogo() const
{
    return _skinColors.value("mainWalletLogo").toString();
}

//==============================================================================

QString SkinColors::walletSideMenuLogo() const
{
    return _skinColors.value("walletSideMenuLogo").toString();
}

//==============================================================================

QColor SkinColors::secondaryBackground() const
{
    return getColor("secondaryBackground");
}

//==============================================================================

QColor SkinColors::mainText() const
{
    return getColor("mainText");
}

//==============================================================================

QColor SkinColors::secondaryText() const
{
    return getColor("secondaryText");
}

//==============================================================================

QColor SkinColors::menuBackground() const
{
    return getColor("menuBackground");
}

//==============================================================================

QColor SkinColors::menuItemSelectedText() const
{
    return getColor("menuItemSelectedText");
}

//==============================================================================

QColor SkinColors::menuItemText() const
{
    return getColor("menuItemText");
}

//==============================================================================

QColor SkinColors::menuItemContainsMouseText() const
{
    return getColor("menuItemContainsMouseText");
}

//==============================================================================

QColor SkinColors::menuItemSelectedBackground() const
{
    return getColor("menuItemSelectedBackground");
}

//==============================================================================

QColor SkinColors::headerText() const
{
    return getColor("headerText");
}

//==============================================================================

QColor SkinColors::headerBackground() const
{
    return getColor("headerBackground");
}

//==============================================================================

QColor SkinColors::slideBarBackground() const
{
    return getColor("slideBarBackground");
}

//==============================================================================

QColor SkinColors::buttonsIcon() const
{
    return getColor("buttonsIcon");
}

//==============================================================================

QColor SkinColors::magnifyingGlass() const
{
    return getColor("magnifyingGlass");
}

//==============================================================================

QColor SkinColors::mobileButtonBackground() const
{
    return getColor("mobileButtonBackground");
}

//==============================================================================

QColor SkinColors::mobileWalletReceive() const
{
    return getColor("mobileWalletReceive");
}

//==============================================================================

QColor SkinColors::mobileLineSeparator() const
{
    return getColor("mobileLineSeparator");
}

//==============================================================================

QColor SkinColors::mobileTopGradientBackground() const
{
    return getColor("mobileTopGradientBackground");
}

//==============================================================================

QColor SkinColors::mobileMiddleGradientBackground() const
{
    return getColor("mobileMiddleGradientBackground");
}

//==============================================================================

QColor SkinColors::mobileBottomGradientBackground() const
{
    return getColor("mobileBottomGradientBackground");
}

//==============================================================================

QColor SkinColors::mobileTitle() const
{
    return getColor("mobileTitle");
}

//==============================================================================

QColor SkinColors::mobileSecondaryBackground() const
{
    return getColor("mobileSecondaryBackground");
}

//==============================================================================

QColor SkinColors::mobileIconButtonBackground() const
{
    return getColor("mobileSecondaryBackground");
}

//==============================================================================

QColor SkinColors::mobileActiveSwitchBackground() const
{
    return getColor("mobileActiveSwitchBackground");
}

//==============================================================================

QColor SkinColors::mobileDisactiveSwitchBackground() const
{
    return getColor("mobileDisactiveSwitchBackground");
}

//==============================================================================

QColor SkinColors::mobileActiveSwitchIndicator() const
{
    return getColor("mobileActiveSwitchIndicator");
}

//==============================================================================

QColor SkinColors::mobileDisactiveSwitchIndicator() const
{
    return getColor("mobileDisactiveSwitchIndicator");
}

//==============================================================================

QColor SkinColors::mobileLocalizationSelectedItemBackground() const
{
    return getColor("mobileLocalizationSelectedItemBackground");
}

//==============================================================================

QColor SkinColors::mobileSettingsSecondaryBackground() const
{
    return getColor("mobileSettingsSecondaryBackground");
}

//==============================================================================

QColor SkinColors::mobileWalletItemBackground() const
{
    return getColor("mobileWalletItemBackground");
}

//==============================================================================

QColor SkinColors::mobileRestoreAdviceText() const
{
    return getColor("mobileRestoreAdviceText");
}

//==============================================================================

QColor SkinColors::dexPageBackground() const
{
    return getColor("dexPageBackground");
}

//==============================================================================

QColor SkinColors::mainDexViewBackgroundGradient1() const
{
    return getColor("mainDexViewBackgroundGradient1");
}

//==============================================================================

QColor SkinColors::mainDexViewBackgroundGradient2() const
{
    return getColor("mainDexViewBackgroundGradient2");
}

//==============================================================================

QColor SkinColors::dexPageSecondaryBackground() const
{
    return getColor("dexPageSecondaryBackground");
}

//==============================================================================

QColor SkinColors::dexBalancesViewBackground() const
{
    return getColor("dexBalancesViewBackground");
}

//==============================================================================

QColor SkinColors::dexPercentViewSeparator() const
{
    return getColor("dexPercentViewSeparator");
}

//==============================================================================

QColor SkinColors::dexSubHeaderBackgroundGradient1() const
{
    return getColor("dexSubHeaderBackgroundGradient1");
}

//==============================================================================

QColor SkinColors::dexSubHeaderBackgroundGradient2() const
{
    return getColor("dexSubHeaderBackgroundGradient2");
}

//==============================================================================

void SkinColors::initSkin(QString skinFilePath, QString skinName)
{
    QString localFilePath = QUrl(skinFilePath).toLocalFile();
    bool isLocalFile = localFilePath.isEmpty();

    QString filePath = isLocalFile ? skinFilePath : QUrl(skinFilePath).toLocalFile();

    if (!isLocalFile && !_fileSystemWatcher->files().contains(filePath)) {
        _fileSystemWatcher->addPath(filePath);
    }

    QFile file(filePath);

    if (file.open(QFile::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        _skinColors = doc.object();

        if (isLocalFile) {
            _skinFilePath = filePath;
            _skinName = skinName;
            writeSettings();
        }

        colorChanged();
    } else {
        LogCritical() << "Failed to open basicSkin.json";
        throw std::runtime_error("Failed to open basicSkin.json");
    }
}

//==============================================================================

bool SkinColors::isActiveSkin(QString skinFilePath)
{
    return _skinFilePath == skinFilePath;
}

//==============================================================================

void SkinColors::initCustomSkin(QString skinFilePath)
{
    initSkin(skinFilePath, "Custom");
}

//==============================================================================

QColor SkinColors::transactionItemSelectedBackground() const
{
    return getColor("transactionItemSelectedBackground");
}

//==============================================================================

QColor SkinColors::transactionItemSecondaryText() const
{
    return getColor("transactionItemSecondaryText");
}

//==============================================================================

QColor SkinColors::transactionItemSent() const
{
    return getColor("transactionItemSent");
}

//==============================================================================

QColor SkinColors::transactionItemReceived() const
{
    return getColor("transactionItemReceived");
}

//==============================================================================

QColor SkinColors::walletAssetsBackgroundGradient() const
{
    return getColor("walletAssetsBackgroundGradient");
}

//==============================================================================

QColor SkinColors::walletButtonsBackgroundGradient() const
{
    return getColor("walletButtonsBackgroundGradient");
}

//==============================================================================

QColor SkinColors::comboboxIndicatorColor() const
{
    return getColor("comboboxIndicatorColor");
}

//==============================================================================

QColor SkinColors::settingsPageHeaderGradient() const
{
    return getColor("settingsPageHeaderGradient");
}

//==============================================================================

QColor SkinColors::settingPageHeaderText() const
{
    return getColor("settingPageHeaderText");
}

//==============================================================================

QColor SkinColors::settingPageHeaderCurrentText() const
{
    return getColor("settingPageHeaderCurrentText");
}

//==============================================================================

QColor SkinColors::settingsAssetsBackground() const
{
    return getColor("settingsAssetsBackground");
}

//==============================================================================

QColor SkinColors::settingsActiveAsset() const
{
    return getColor("settingsActiveAsset");
}

//==============================================================================

QColor SkinColors::settingsShareIcons() const
{
    return getColor("settingsShareIcons");
}

//==============================================================================

QColor SkinColors::settingPagePersonalTabTextArea() const
{
    return getColor("settingPagePersonalTabTextArea");
}

//==============================================================================

QColor SkinColors::assetsRoundedCheckboxBorder() const
{
    return getColor("assetsRoundedCheckboxBorder");
}

QColor SkinColors::assetsMenuBackground() const
{
    return getColor("assetsMenuBackground");
}

//==============================================================================

QColor SkinColors::lightningPageInfoText() const
{
    return getColor("lightningPageInfoText");
}

//==============================================================================

QColor SkinColors::lightningPageWarningGradientFirst() const
{
    return getColor("lightningPageWarningGradientFirst");
}

//==============================================================================

QColor SkinColors::lightningPageWarningGradientSecond() const
{
    return getColor("lightningPageWarningGradientSecond");
}

//==============================================================================

QColor SkinColors::menuBackgroundGradientFirst() const
{
    return getColor("menuBackgroundGradientFirst");
}

//==============================================================================

QColor SkinColors::menuBackgroundGradientSecond() const
{
    return getColor("menuBackgroundGradientSecond");
}

//==============================================================================

QColor SkinColors::menuBackgroundGradientThird() const
{
    return getColor("menuBackgroundGradientThird");
}

//==============================================================================

QColor SkinColors::menuBackgroundGradienRightLine() const
{
    return getColor("menuBackgroundGradienRightLine");
}

//==============================================================================

QColor SkinColors::popupFieldBorder() const
{
    return getColor("popupFieldBorder");
}

//==============================================================================

QColor SkinColors::popupInfoText() const
{
    return getColor("popupInfoText");
}

//==============================================================================

QColor SkinColors::sendPopupConfirmText() const
{
    return getColor("sendPopupConfirmText");
}

//==============================================================================

QColor SkinColors::highlightedItemHeader() const
{
    return getColor("highlightedItemHeader");
}

//==============================================================================

QColor SkinColors::highlightedMenuItem() const
{
    return getColor("highlightedMenuItem");
}

//==============================================================================

QColor SkinColors::highlightedAssetsItem() const
{
    return getColor("highlightedAssetsItem");
}

//==============================================================================

QColor SkinColors::walletButtonsHoveredBackgroundGradient() const
{
    return getColor("walletButtonsHoveredBackgroundGradient");
}

//==============================================================================

QColor SkinColors::menuBackgroundHoveredGradienRightLine() const
{
    return getColor("menuBackgroundHoveredGradienRightLine");
}

//==============================================================================

QColor SkinColors::errorViewBackground() const
{
    return getColor("errorViewBackground");
}

//==============================================================================

QColor SkinColors::orderBookEvenBackground() const
{
    return getColor("orderBookEvenBackground");
}

//==============================================================================

QColor SkinColors::introBtnHoveredColor() const
{
    return getColor("introBtnHoveredColor");
}

//==============================================================================

QColor SkinColors::introBtnColor() const
{
    return getColor("introBtnColor");
}

//==============================================================================

QColor SkinColors::introBtnGradientHoveredColor() const
{
    return getColor("introBtnGradientHoveredColor");
}

//==============================================================================

QColor SkinColors::introBtnGradientColor() const
{
    return getColor("introBtnGradientColor");
}

//==============================================================================

QColor SkinColors::popupsBgColor() const
{
    return getColor("popupsBgColor");
}

//==============================================================================

QColor SkinColors::switchActiveIndicatorColor() const
{
    return getColor("switchActiveIndicatorColor");
}

//==============================================================================

QColor SkinColors::switchActiveIndicatorBorderColor() const
{
    return getColor("switchActiveIndicatorBorderColor");
}

//==============================================================================

QColor SkinColors::notificationViewBackground() const
{
    return getColor("notificationViewBackground");
}

//==============================================================================

QColor SkinColors::activeNotificationItem() const
{
    return getColor("activeNotificationItem");
}

//==============================================================================

QColor SkinColors::buttonBorderColor() const
{
    return getColor("buttonBorderColor");
}

//==============================================================================

QColor SkinColors::dexDisclaimerTextColor() const
{
    return getColor("dexDisclaimerTextColor");
}

//==============================================================================

QColor SkinColors::cancelButtonHoveredBgrColor() const
{
    return getColor("cancelButtonHoveredBgrColor");
}

//==============================================================================

QColor SkinColors::cancelButtonColor() const
{
    return getColor("cancelButtonColor");
}

//==============================================================================

QColor SkinColors::cancelButtonGradientHoveredColor() const
{
    return getColor("cancelButtonGradientHoveredColor");
}

//==============================================================================

QColor SkinColors::cancelButtonGradienColor() const
{
    return getColor("cancelButtonGradienColor");
}

//==============================================================================

QColor SkinColors::swapPageBackgroundHeaderFirst() const
{
    return getColor("swapPageBackgroundHeaderFirst");
}

//==============================================================================

QColor SkinColors::swapPageBackgroundHeaderSecond() const
{
    return getColor("swapPageBackgroundHeaderSecond");
}

//==============================================================================

QColor SkinColors::swapPageBackgroundHeaderThird() const
{
    return getColor("swapPageBackgroundHeaderThird");
}

//==============================================================================

QColor SkinColors::swapPageBackgroundFooterFirst() const
{
    return getColor("swapPageBackgroundFooterFirst");
}

//==============================================================================

QColor SkinColors::swapPageBackgroundFooterSecond() const
{
    return getColor("swapPageBackgroundFooterSecond");
}

//==============================================================================

QColor SkinColors::swapPageSecondaryBackground() const
{
    return getColor("swapPageSecondaryBackground");
}

//==============================================================================

QColor SkinColors::botPageBackground() const
{
    return getColor("botPageBackground");
}

//==============================================================================

QColor SkinColors::botPageBackgroundImageColorFirst() const
{
    return getColor("botPageBackgroundImageColorFirst");
}

//==============================================================================

QColor SkinColors::botPageBackgroundImageColorSecond() const
{
    return getColor("botPageBackgroundImageColorSecond");
}

//==============================================================================

QColor SkinColors::botPageBackgroundImageColorThird() const
{
    return getColor("botPageBackgroundImageColorThird");
}

//==============================================================================

QColor SkinColors::botPageMainBackgroundColorFirst() const
{
    return getColor("botPageMainBackgroundColorFirst");
}

//==============================================================================

QColor SkinColors::botPageMainBackgroundColorThird() const
{
    return getColor("botPageMainBackgroundColorThird");
}

//==============================================================================

QColor SkinColors::botPageMainBackgroundColorSecond() const
{
    return getColor("botPageMainBackgroundColorSecond");
}

//==============================================================================

QColor SkinColors::botDepthCurveImageBackground() const
{
    return getColor("botDepthCurveImageBackground");
}

//==============================================================================

QColor SkinColors::botAssetImageBackground() const
{
    return getColor("botAssetImageBackground");
}

//==============================================================================

QColor SkinColors::botTextFieldActiveStateColor() const
{
    return getColor("botTextFieldActiveStateColor");
}

//==============================================================================

QColor SkinColors::botTextFieldInactiveStateColor() const
{
    return getColor("botTextFieldInactiveStateColor");
}

//==============================================================================

QColor SkinColors::botTextFieldActiveBorderColor() const
{
    return getColor("botTextFieldActiveBorderColor");
}

//==============================================================================

QColor SkinColors::botTextFieldInactiveBorderColor() const
{
    return getColor("botTextFieldInactiveBorderColor");
}

//==============================================================================

QColor SkinColors::botComboBoxHighlightedColor() const
{
    return getColor("botComboBoxHighlightedColor");
}

//==============================================================================

QColor SkinColors::botComboBoxBackgroundColor() const
{
    return getColor("botComboBoxBackgroundColor");
}

//==============================================================================

QColor SkinColors::botComboBoxPopupBackgroundColor() const
{
    return getColor("botComboBoxPopupBackgroundColor");
}

//==============================================================================

QColor SkinColors::botComboBoxPopupBackgroundBorderColor() const
{
    return getColor("botComboBoxPopupBackgroundBorderColor");
}

//==============================================================================

QColor SkinColors::botSliderBackground() const
{
    return getColor("botSliderBackground");
}

//==============================================================================

QColor SkinColors::botSliderBackgroundColorFirst() const
{
    return getColor("botSliderBackgroundColorFirst");
}

//==============================================================================

QColor SkinColors::botSliderBackgroundColorSecond() const
{
    return getColor("botSliderBackgroundColorSecond");
}

//==============================================================================

QColor SkinColors::botSliderHandlePressedColor() const
{
    return getColor("botSliderHandlePressedColor");
}

//==============================================================================

QColor SkinColors::botSliderHandleInactiveColor() const
{
    return getColor("botSliderHandleInactiveColor");
}

//==============================================================================

QColor SkinColors::botSliderHandleBorderColor() const
{
    return getColor("botSliderHandleBorderColor");
}

//==============================================================================

QColor SkinColors::botSliderHandleLines() const
{
    return getColor("botSliderHandleLines");
}

//==============================================================================

QColor SkinColors::swapPageBackgroundFooterThird() const
{
    return getColor("swapPageBackgroundFooterThird");
}

//==============================================================================

QColor SkinColors::swapComboBoxBackground() const
{
    return getColor("swapComboBoxBackground");
}

//==============================================================================

QColor SkinColors::walletPageBackgroundLightColor() const
{
    return getColor("walletPageBackgroundLightColor");
}

//==============================================================================

QColor SkinColors::walletPageHeaderViewBlueColor() const
{
    return getColor("walletPageHeaderViewBlueColor");
}

//==============================================================================

QColor SkinColors::delegatesBackgroundLightColor() const
{
    return getColor("delegatesbackGroundLightColor");
}

//==============================================================================

QColor SkinColors::delegatesBackgroundDarkColor() const
{
    return getColor("delegatesBackgroundDarkColor");
}

//==============================================================================

QColor SkinColors::tabDelegateLightColor() const
{
    return getColor("tabDelegateLightColor");
}

//==============================================================================

QColor SkinColors::tabDelegateDarkColor() const
{
   return getColor("tabDelegateDarkColor");
}

//==============================================================================

QColor SkinColors::walletAssetHighlightColor() const
{
   return getColor("walletAssetHighlightColor");
}

//==============================================================================

QColor SkinColors::getColor(QString name) const
{
    return (QColor(_skinColors.value(name).toString()));
}

//==============================================================================

void SkinColors::writeSettings() const
{
    QSettings settings;
    settings.setValue(SETTINGS_ACTIVE_SKIN_PATH, QVariant::fromValue(_skinFilePath));
    settings.setValue(SETTINGS_ACTIVE_SKIN_NAME, QVariant::fromValue(_skinName));
    settings.sync();
}

//==============================================================================

void SkinColors::readSettings()
{
    QSettings settings;
    QString skinFilePath = settings.value(SETTINGS_ACTIVE_SKIN_PATH).value<QString>();
    QString skinName = settings.value(SETTINGS_ACTIVE_SKIN_NAME).value<QString>();
    if (skinFilePath.isEmpty() && skinName.isEmpty()) {
        _skinFilePath = ":/data/basicSkin.json";
        _skinName = "Default";
    } else {
        _skinFilePath = skinFilePath;
        _skinName = skinName;
    }
}

//==============================================================================
