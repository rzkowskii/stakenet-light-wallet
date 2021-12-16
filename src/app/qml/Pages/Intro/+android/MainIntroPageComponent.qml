import QtQuick 2.4
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2

import Components 1.0

import com.xsn.utils 1.0
import com.xsn.viewmodels 1.0

ColumnLayout {
    Layout.alignment: Qt.AlignHCenter
    spacing: 70

    FontLoader { id: regularFont; source: "qrc:/Rubik-Regular.ttf" }

    ColumnLayout {
        Layout.topMargin: 60
        Layout.leftMargin: 25
        Layout.rightMargin: 25
        Layout.fillWidth: true
        spacing: 30

        Image {
            Layout.alignment: Qt.AlignHCenter
            source: "qrc:/images/ic_stnetDX.png"
            sourceSize: Qt.size(85, 97)
        }

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: "Stakenet DEX"
            color: "white"
            font.pixelSize: 24
            font.family: regularFont.name
            font.weight: Font.Medium
        }


        Item {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter

            SecondaryLabel {
                width: parent.width
                lineHeight: 1.3
                text: "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nam urna erat, placerat a interdum, auctor ac lorem. In sit amet gravida sem. Cras nec lectus sit vehicula mollis."
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                font.pixelSize: 14
            }
        }
    }

    Item {
        Layout.fillHeight: true
    }

    ColumnLayout {
        Layout.fillWidth: true
        Layout.leftMargin: 35
        Layout.rightMargin: 35
        Layout.bottomMargin: 65
        Layout.alignment: Qt.AlignHCenter
        spacing: 42

        MobileActionButton {
            checkable: true
            Layout.preferredWidth: 287
            Layout.preferredHeight: 55
            Layout.alignment: Qt.AlignHCenter

            source: "qrc:/images/ic_new_wallet.png"
            buttonColor: SkinColors.menuBackgroundGradientFirst
            buttonText: qsTr("Get Started")
            buttonRadius: 27
            buttonSpacing: 15
            iconWidth: 20
            iconHeight: 17
            context.font.capitalization: Font.MixedCase
            context.font.pixelSize: 16

            PointingCursorMouseArea {
                onClicked: stackView.push(setupNewWalletComponent)//ApplicationViewModel.walletViewModel.createWalletWithMnemonic();
            }
        }

        MobileActionButton {
            checkable: true
            Layout.preferredWidth: 287
            Layout.preferredHeight: 55
            Layout.alignment: Qt.AlignHCenter

            source: "qrc:/images/ic_restore_2.png"
            buttonColor: SkinColors.menuBackgroundGradientFirst
            buttonText: qsTr("Restore your Wallet")
            buttonRadius: 27
            buttonSpacing: 15
            iconWidth: 19
            iconHeight: 25
            context.font.capitalization: Font.MixedCase
            context.font.pixelSize: 16

            PointingCursorMouseArea {
                onClicked: navigateToItem(restoreFromBackupComponent, {isRestoreFromExistWallet : false})
            }
        }
    }
}
