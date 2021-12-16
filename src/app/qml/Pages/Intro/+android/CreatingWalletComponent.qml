import QtQuick 2.4
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2

import Components 1.0
import Popups 1.0
import Pages 1.0
import Intro 1.0

import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

Item {

    Component.onCompleted: {
        ApplicationViewModel.walletViewModel.createWalletWithMnemonic()
    }

    FontLoader {
        id: regularFont
        source: "qrc:/Rubik-Regular.ttf"
    }

    Connections {
        target: ApplicationViewModel.walletViewModel
        function onCreatedChanged(isCreated) {
            if (isCreated) {
                stackLayout.currentIndex = 1
            } else {
                stackLayout.currentIndex = 0
            }
        }
    }

    StackLayout {
        id: stackLayout
        anchors.fill: parent

        Item {

            Row {
                anchors.centerIn: parent
                spacing: 10

                XSNLabel {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Creating wallet")
                    font.pixelSize: 24
                }

                BusyIndicator {
                    running: true
                }
            }
        }

        Item {
            ColumnLayout {
                anchors.centerIn: parent
                Layout.alignment: Qt.AlignHCenter
                spacing: 70

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    color: SkinColors.mainText
                    text: "Wallet created!"
                    font.pixelSize: 18
                    font.family: regularFont.name
                    font.weight: Font.Medium
                }

                MobileActionButton {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredHeight: 41
                    Layout.preferredWidth: 200
                    buttonColor: SkinColors.menuBackgroundGradientFirst
                    buttonText: "OK"
                    onClicked: replaceView(mainPage)
                }
            }
        }
    }
}
