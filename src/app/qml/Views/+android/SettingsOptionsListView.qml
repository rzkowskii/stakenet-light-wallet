import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0

import Components 1.0
import Pages 1.0
import Views 1.0

import com.xsn.utils 1.0
import com.xsn.viewmodels 1.0

ListView {
    id: settingsListView
    Layout.bottomMargin: 7

   Component {
       id: mobileSettingsAssetsComponent
       MobileSettingsAssetsPage {
    }
   }

   Component {
       id: mobileRestorePreviewComponent
       MobileRestorePreview {

       }
   }
   Component {
       id: mobilePersonalizeComponent
       MobilePersonalizePage {

       }
   }
   Component {
       id: mobileHelpComponent
       MobileHelpPage {

       }
   }

    model: ListModel {
        ListElement { name: "Assets"; image: "qrc:/images/IC_ASSETS.png"; page: mobileSettingsAssetsComponent }
        ListElement { name: "Backup Wallets"; image: "qrc:/images/IC_BACKUP.png"; page: "" }
        ListElement { name: "Restore Wallets"; image: "qrc:/images/IC_RESTORE.png"; page: mobileRestorePreviewComponent }
        ListElement { name: "Localizations"; image: "qrc:/images/IC_LOCALIZATIONS.png"; page: "" }
        ListElement { name: "Personalize"; image: "qrc:/images/IC_PERSONA.png"; page: mobilePersonalizeComponent }
        ListElement { name: "Help"; image: "qrc:/images/IC_HELP.png"; page: mobileHelpComponent }
    }

    property int actualIndex: 0
    clip: true
    spacing: 7
    boundsBehavior: Flickable.StopAtBounds

    delegate: Item  {
        id: settingsItem
        height: 57
        width: parent.width
        property bool isLocalizations: model.name === "Localizations"

        Rectangle {
            id: backGrd
            anchors.fill: parent
            opacity: mouseArea.containsMouse ? 0.3 : 0.6
            radius: 8
            color: mouseArea.containsMouse ? SkinColors.highlightedMenuItem : SkinColors.mobileSettingsSecondaryBackground
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.rightMargin: 20
            spacing: 15

            Image {
                Layout.alignment: Qt.AlignVCenter
                source: model.image
                sourceSize: Qt.size(21, 21)
            }

            Text {
                id: assetsName
                Layout.alignment: Qt.AlignVCenter
                text: model.name
                font.pixelSize: 14
                font.family: lightFont.name
                color: "white"
            }

            Item {
                Layout.fillHeight: true
                Layout.fillWidth: true
            }

            Text {
                visible: isLocalizations
                Layout.alignment: Qt.AlignVCenter
                text: "%1 %2" .arg(ApplicationViewModel.localCurrencyViewModel.currentCurrencySymbol) .arg(ApplicationViewModel.localCurrencyViewModel.currentCurrencyCode)
                font.capitalization: Font.AllUppercase
                font.pixelSize: 14
                font.family: lightFont.name
                color: SkinColors.menuBackgroundGradientFirst
            }
        }

        PointingCursorMouseArea {
            id: mouseArea
            onClicked: {
                if(isLocalizations)
                {
                   openMobileLocalizationDialog({});
                }

                if(model.page)
                {
                    navigateToItem(model.page)
                }
            }
        }
    }
}
