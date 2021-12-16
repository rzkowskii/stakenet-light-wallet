import QtQuick 2.4
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2

import Components 1.0

import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

StackViewPage {
    id: loadingWalletPage
    headerText: "STAKENET | DEX"
    headerSource: SkinColors.mainWalletLogo
    //    backButton.visible: false
    skipButton.visible: false

    backButton.onClicked: {
        stackView.pop()
    }

    property string password: ""

    background: Image {
        anchors.fill: parent
        source: "qrc:/images/ComingSoonPageBg.svg"
    }

    Component.onCompleted: {
        ApplicationViewModel.walletViewModel.createWalletWithMnemonic()
    }

    Connections {
        target: ApplicationViewModel.walletViewModel
        function onCreatedChanged(isCreated) {
            if (isCreated) {
                if (password != "") {
                    ApplicationViewModel.walletViewModel.encryptWallet(password)
                } else {
                    stackLayout.currentIndex = 1
                }
            } else {
                stackLayout.currentIndex = 0
            }
        }
        function onWalletEncrypted() {
            stackLayout.currentIndex = 1
        }
        function onEncryptWalletFailed(error) {}
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

        RowLayout {
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.topMargin: 30
            Layout.bottomMargin: 52

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            Rectangle {
                Layout.preferredWidth: 424
                Layout.preferredHeight: 521
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
                        text: "Backup your wallet"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 22
                        font.bold: true
                    }

                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: 338
                        text: "In the next step you will see your wallet's 24-word seed phrase. Re-entering these 24 words in the order in which they appear will be the only way to recover your wallet should you lose access to it."
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        wrapMode: Text.WordWrap
                        font.pixelSize: 15
                        color: "#7377A5"
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 54
                        Layout.topMargin: 10
                        spacing: 20

                        IntroButton {
                            Layout.leftMargin: 58
                            Layout.preferredWidth: 142
                            Layout.preferredHeight: 48
                            Layout.bottomMargin: 54
                            radius: 8
                            text: "Backup later"
                            textColor: "white"
                            borderColor: "transparent"
                            buttonColor: "#331254DD"
                            buttonGradientColor: "#331D96EC"
                            buttonHoveredColor: "#331254DD"
                            buttonGradientHoveredColor: "#331D96EC"

                            onClicked: {
                                stackView.push(backupLaterComponent)
                            }
                        }

                        IntroButton {
                            Layout.rightMargin: 58
                            Layout.preferredWidth: 142
                            Layout.preferredHeight: 48
                            Layout.bottomMargin: 54
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
}
