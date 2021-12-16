import QtQuick 2.6
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12

import Components 1.0

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

    RowLayout {
        anchors.fill: parent
        anchors.topMargin: 37
        anchors.bottomMargin: 52

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        Rectangle {
            Layout.preferredWidth: 524
            Layout.preferredHeight: 580
            Layout.alignment: Qt.AlignVCenter
            color: "#99151B30"

            ColumnLayout {
                anchors.fill: parent
                spacing: 20

                Image {
                    source: "qrc:/images/into_page_ic_bg.svg"
                    sourceSize: Qt.size(196, 222)
                    Layout.alignment: Qt.AlignHCenter

                    Image {
                        source: "qrc:/images/ic_cloud_restore.svg"
                        sourceSize: Qt.size(70, 49)
                        anchors.centerIn: parent
                    }
                }

                XSNLabel {
                    Layout.alignment: Qt.AlignHCenter
                    text: "Restore From Backup"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 22
                    font.bold: true
                }

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "Type your 24 word seed phrase!"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 15
                    color: "#7377A5"
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 120
                    Layout.leftMargin: 40
                    Layout.rightMargin: 40

                    color: SkinColors.settingPagePersonalTabTextArea
                    border.color: "#7377A5"
                    border.width: 1

                    EnterMnemonicTextArea {
                        id: mnemonic
                        anchors.top: parent.top
                        width: parent.width
                        font.bold: false
                        font.pixelSize: 14
                        color: SkinColors.mainText
                        placeholderText: "Enter your seed ..."

                        background: Rectangle {
                            color: "transparent"
                        }

                        Menu {
                            id: contextMenu

                            Action {
                                text: qsTr("Paste")
                                enabled: mnemonic.canPaste
                                onTriggered: mnemonic.paste()
                            }

                            delegate: MenuItemDelegate {
                                id: menuItem
                            }

                            background: Rectangle {
                                implicitWidth: 180
                                implicitHeight: 30
                                color: SkinColors.menuBackground
                                border.color: SkinColors.popupFieldBorder
                            }
                        }
                    }

                    StyledMouseArea {
                        id: styledMouseArea
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton | Qt.RightButton

                        onClicked: {
                            if (mouse.button & Qt.RightButton) {
                                curPos = mnemonic.cursorPosition
                                contextMenu.x = mouse.x
                                contextMenu.y = mouse.y
                                contextMenu.open()
                                mnemonic.cursorPosition = curPos
                            } else {
                                mnemonic.forceActiveFocus()
                                mnemonic.cursorPosition = mnemonic.positionAt(
                                            mouse.x, mouse.y)
                            }
                        }
                    }
                }

                XSNLabel {
                    Layout.preferredHeight: 30
                    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                    Layout.rightMargin: 40
                    text: "What is your 24 word seed phrase?"
                    font.underline: true
                    font.pixelSize: 16

                    PointingCursorMouseArea {
                        onClicked: openNotificationDialog(
                                       "24 word seed phrase",
                                       "A seed phrase, seed recovery phrase or backup seed phrase is a list of words which store all the information needed to recover a wallet, and recover any private keys associated with given wallet.")
                    }
                }

                IntroButton {
                    Layout.preferredWidth: 180
                    Layout.preferredHeight: 40
                    Layout.bottomMargin: 50
                    Layout.alignment: Qt.AlignHCenter
                    radius: 8
                    text: "Continue"
                    textColor: "white"
                    borderColor: "transparent"
                    buttonColor: "#1254DD"
                    buttonGradientColor: "#1D96EC"
                    buttonHoveredColor: "#1254DD"
                    buttonGradientHoveredColor: "#1D96EC"

                    onClicked: {
                        ApplicationViewModel.walletViewModel.restoreWallet(
                                    mnemonic.text, "")
                        stackView.push(restoringWalletComponent)
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
