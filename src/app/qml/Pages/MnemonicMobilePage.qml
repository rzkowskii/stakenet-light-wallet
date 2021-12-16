import QtQuick 2.4
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2

import Components 1.0
import Popups 1.0
import Pages 1.0
import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

Page {
    id: root

    background: Rectangle {
        color: "transparent"
    }

    FontLoader { id: regularFont; source: "qrc:/Rubik-Regular.ttf" }
    Component.onCompleted: ApplicationViewModel.walletViewModel.requestMnemonic()

    Connections {
        target: ApplicationViewModel.walletViewModel
        function onRequestMnemonicFinished(mnemonic) {
            elements.model = mnemonic.split(" ")
        }
    }

    ColumnLayout {
        id: layout
        Layout.alignment: Qt.AlignHCenter
        spacing: 40
        anchors.fill: parent
        anchors.topMargin: 35
        anchors.bottomMargin: 40
        anchors.leftMargin: 16
        anchors.rightMargin: 16

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: "24 Words Seeds"
            font.family: regularFont.name
            font.pixelSize: 14
            color: SkinColors.mainText
            font.weight: Font.Medium
        }

        Flow {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            spacing: 7

            Repeater {
                id: elements
                anchors.fill: parent
                anchors.horizontalCenter: parent.horizontalCenter

                delegate: Item {
                    width: mnemonicWord.contentWidth + 30
                    height: 41

                    Rectangle {
                        anchors.fill: parent
                        color: SkinColors.mobileSecondaryBackground
                        opacity: 0.6
                        radius: 20
                    }

                    SecondaryLabel {
                        id: mnemonicWord
                        text: modelData !== undefined ? modelData : ""
                        anchors.centerIn: parent
                        font.family: regularFont.name
                        font.pixelSize: 12
                    }
                }
            }
        }

        SecondaryLabel {
            Layout.leftMargin: 5
            Layout.rightMargin: 5
            Layout.fillWidth: true
            Layout.preferredHeight: 35
            Layout.alignment: Qt.AlignHCenter
            horizontalAlignment: Text.AlignHCenter
            font.family: regularFont.name
            font.pixelSize: 12
            wrapMode: Text.WordWrap
            text: "Please write down your 24 words and store it somewhere to make you didn`t lose it"
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 50

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignHCenter

                MobileIconButton {
                    anchors.centerIn: parent
                    color: SkinColors.mobileButtonBackground
                    image.source: "qrc:/images/ic_clone.png"
                    border.width: 1
                    border.color: SkinColors.menuBackgroundGradientFirst
                    onClicked: {
                        Clipboard.setText( elements.model.join(" "));
                        mainWindow.showBubblePopup("Copied");
                    }
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignHCenter

                MobileIconButton {
                    anchors.centerIn: parent
                    color: SkinColors.mobileButtonBackground
                    image.source: "qrc:/images/ic_print.png"
                    border.width: 1
                    border.color: SkinColors.menuBackgroundGradientFirst
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignHCenter

                MobileIconButton {
                    anchors.centerIn: parent
                    color: SkinColors.mobileButtonBackground
                    image.source: "qrc:/images/ic_email.png"
                    border.width: 1
                    border.color: SkinColors.menuBackgroundGradientFirst
                }
            }
        }

        MobileActionButton {
            Layout.leftMargin: 6
            Layout.rightMargin: 6
            Layout.preferredHeight: 41
            Layout.fillWidth: true
            buttonColor: SkinColors.menuBackgroundGradientFirst
            buttonText: "Done"
            onClicked: navigateBack()
        }
    }
}
