import QtQuick 2.4
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2

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
        anchors.topMargin: 30
        anchors.bottomMargin: 52

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        Rectangle {
            Layout.preferredWidth: 494
            Layout.preferredHeight: 521
            Layout.alignment: Qt.AlignVCenter
            color: "#99151B30"

            ColumnLayout {
                anchors.fill: parent
                spacing: 20

                Image {
                    source: "qrc:/images/ic_pwdbig.svg"
                    sourceSize: Qt.size(170, 193)
                    Layout.alignment: Qt.AlignHCenter
                }

                XSNLabel {
                    Layout.alignment: Qt.AlignHCenter
                    text: "Create wallet without backup?"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 19
                    font.bold: true
                }

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "You could lose all your funds."
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 15
                    color: "#7377A5"
                }

                WarningComponent {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredHeight: 70
                    Layout.preferredWidth: 400
                    message: qsTr("You could lose all your funds, if you want to restore your wallet without your seed written down carefully.")
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 54
                    Layout.topMargin: 20
                    spacing: 17

                    IntroButton {
                        Layout.leftMargin: 30
                        Layout.preferredWidth: 211
                        Layout.preferredHeight: 48
                        Layout.bottomMargin: 40
                        radius: 8
                        text: "Create Wallet"
                        textColor: "white"
                        borderColor: "transparent"
                        buttonColor: "#331254DD"
                        buttonGradientColor: "#331D96EC"
                        buttonHoveredColor: "#331254DD"
                        buttonGradientHoveredColor: "#331D96EC"

                        onClicked: {
                            stackView.push(walletCreatedComponent, {
                                               "mainHeaderLbl": "Wallet Created",
                                               "secondaryHeaderText": "Your wallet has been successfully created. Thank you for using Stakenet DEX!"
                                           })
                        }
                    }

                    IntroButton {
                        Layout.rightMargin: 30
                        Layout.preferredWidth: 211
                        Layout.preferredHeight: 48
                        Layout.bottomMargin: 40
                        radius: 8
                        text: "Generate seed"
                        textColor: "white"
                        borderColor: "transparent"
                        buttonColor: "#1254DD"
                        buttonGradientColor: "#1D96EC"
                        buttonHoveredColor: "#1254DD"
                        buttonGradientHoveredColor: "#1D96EC"

                        onClicked: {
                            stackView.push(backupWalletComponent)
                        }
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
