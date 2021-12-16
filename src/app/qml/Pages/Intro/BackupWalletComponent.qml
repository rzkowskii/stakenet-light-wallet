import QtQuick 2.4
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2

import Components 1.0
import Popups 1.0
import Pages 1.0
import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

StackViewPage {
    skipButton.visible: false
    headerText: "STAKENET | DEX"
    headerSource: SkinColors.mainWalletLogo
    backButton.onClicked: {
        stackView.pop()
    }

    background: Image {
        anchors.fill: parent
        source: "qrc:/images/ComingSoonPageBg.svg"
    }

    property var mnemonicList
    property string mnemonicPhrase: ""

    Component.onCompleted: ApplicationViewModel.walletViewModel.requestMnemonic(
                               )

    Connections {
        target: ApplicationViewModel.walletViewModel
        function onRequestMnemonicFinished(mnemonic) {
            mnemonicPhrase = mnemonic
            mnemonicList = mnemonic.split(' ')
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.topMargin: 30

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        Rectangle {
            Layout.preferredWidth: 960
            Layout.preferredHeight: 559
            Layout.alignment: Qt.AlignVCenter
            color: "#99151B30"

            ColumnLayout {
                anchors.fill: parent
                spacing: 20

                Image {
                    source: "qrc:/images/ic_shield24.svg"
                    sourceSize: Qt.size(170, 193)
                    Layout.alignment: Qt.AlignHCenter
                }

                XSNLabel {
                    Layout.alignment: Qt.AlignHCenter
                    text: "Save your seed phrase"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 19
                    font.bold: true
                }

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "Please take note of the words and their exact order. Store this seed phrase in a safe place, as it will be needed to restore your wallet."
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 15
                    color: "#7377A5"
                }

                GridView {
                    id: grid
                    Layout.preferredHeight: 160
                    Layout.preferredWidth: 720
                    Layout.leftMargin: 30
                    Layout.rightMargin: 20
                    Layout.alignment: Qt.AlignHCenter
                    cellWidth: 118
                    cellHeight: 40

                    model: mnemonicList

                    delegate: Item {
                        id: delegate
                        width: grid.cellWidth
                        height: grid.cellHeight

                        Rectangle {
                            id: basicRec
                            anchors.fill: parent
                            anchors.margins: 5

                            color: "#1FDEE0FF"
                            radius: 20

                            Row {
                                anchors.fill: parent
                                spacing: 10

                                Rectangle {
                                    width: 30
                                    height: 30
                                    radius: width * 0.5
                                    color: "#807377A5"

                                    XSNLabel {
                                        id: labelIndex
                                        font.pixelSize: 16
                                        color: SkinColors.mainText
                                        anchors.centerIn: parent
                                        font.family: regularFont.name
                                        font.bold: true
                                        text: index + 1
                                    }
                                }

                                XSNLabel {
                                    id: label
                                    font.pixelSize: 14
                                    color: SkinColors.menuItemText
                                    anchors.verticalCenter: parent.verticalCenter
                                    horizontalAlignment: Text.AlignRight
                                    font.family: mediumFont.name
                                    text: modelData
                                }
                            }
                        }
                    }
                }

                XSNLabel {
                    id: copySeedLbl
                    Layout.preferredHeight: 10
                    Layout.alignment: Qt.AlignHCenter
                    text: "Copy my seed"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 14

                    font.bold: true
                    color: "#1B8AE9"

                    PointingCursorMouseArea {
                        onClicked: {
                            Clipboard.setText(mnemonicPhrase)
                            showBubblePopup("Copied")
                        }

                        onEntered: {
                            copySeedLbl.color = "#CC1B8AE9"
                        }

                        onExited: {
                            copySeedLbl.color = "#1B8AE9"
                        }
                    }
                }

                IntroButton {
                    Layout.preferredWidth: 250
                    Layout.preferredHeight: 45
                    Layout.bottomMargin: 40
                    Layout.alignment: Qt.AlignHCenter
                    radius: 8
                    text: "Next"
                    textColor: "white"
                    borderColor: "transparent"
                    buttonColor: "#1254DD"
                    buttonGradientColor: "#1D96EC"
                    buttonHoveredColor: "#1254DD"
                    buttonGradientHoveredColor: "#1D96EC"

                    onClicked: {
                        onClicked: stackView.push(verifyMnemonicComponent, {
                                                      "mnemonicPhrase": mnemonicPhrase
                                                  })
                    }
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}
