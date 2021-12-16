import QtQuick 2.4
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2

import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

import "../Components"

Item {
    Layout.fillHeight: true
    Layout.fillWidth: true

    FontLoader { id: regularFont; source: "qrc:/Rubik-Regular.ttf" }
    FontLoader { id: mediumFont; source: "qrc:/Rubik-Medium.ttf" }

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 35
        anchors.bottomMargin: 30
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        spacing: 35

        MobileTitle {
            Layout.alignment: Qt.AlignCenter
            text: "restore wallet"
        }

        Image {
            Layout.alignment: Qt.AlignHCenter
            source: "qrc:/images/artwork_restore.png"
            sourceSize: Qt.size(200, 250)
        }

        ColumnLayout {
            spacing: 25

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: "Restore your Wallets"
                font.pixelSize: 18
                font.family: regularFont.name
                color: SkinColors.mainText
            }

            SecondaryLabel {
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                lineHeight: 1.3
                font.family: regularFont.name
                font.pixelSize: 14
                horizontalAlignment: Text.AlignHCenter
                text: "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Morbi eu elit et dolor elementum molestie. Sed neque massa, rhoncus maximus ipsum nec, bibendum tincidunt justo."
            }
        }

        MobileActionButton {
            Layout.preferredHeight: 41
            Layout.fillWidth: true
            buttonColor: SkinColors.menuBackgroundGradientFirst
            buttonText: "GET STARTED"
            onClicked: ApplicationViewModel.walletViewModel.resetState(WalletViewModel.LoadingState.Done);
        }

        MobileFooter {
            Layout.leftMargin: 10
            Layout.rightMargin: 10
            leftButton.text: "back"
            rightButton.visible: false
            onLeftButtonClicked: {
                navigateBack()
            }
        }
    }
}
