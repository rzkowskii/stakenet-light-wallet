import QtQuick 2.4
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2

import Components 1.0

import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

StackViewPage {
    id: welcomePage
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
            Layout.preferredWidth: 424
            Layout.preferredHeight: 521
            Layout.alignment: Qt.AlignVCenter
            color: "#99151B30"

            ColumnLayout {
                anchors.fill: parent
                spacing: 10

                Image {
                    source: "qrc:/images/welcome_page_assets.png"
                    sourceSize: Qt.size(243, 248)
                    Layout.alignment: Qt.AlignHCenter
                }

                XSNLabel {
                    Layout.alignment: Qt.AlignHCenter
                    text: "Your Future"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 22
                    font.bold: true
                }

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: 338
                    text: "The Stakenet DEX is the one-stop dApp for all your digital asset needs!"
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
                        text: "Back"
                        textColor: "white"
                        borderColor: "transparent"
                        buttonColor: "#331254DD"
                        buttonGradientColor: "#331D96EC"
                        buttonHoveredColor: "#331254DD"
                        buttonGradientHoveredColor: "#331D96EC"

                        onClicked: {
                            stackView.pop()
                        }
                    }

                    IntroButton {
                        Layout.rightMargin: 58
                        Layout.preferredWidth: 142
                        Layout.preferredHeight: 48
                        Layout.bottomMargin: 54
                        radius: 8
                        text: "Continue"
                        textColor: "white"
                        borderColor: "transparent"
                        buttonColor: "#1254DD"
                        buttonGradientColor: "#1D96EC"
                        buttonHoveredColor: "#1254DD"
                        buttonGradientHoveredColor: "#1D96EC"

                        onClicked: {
                            stackView.push(setupNewWalletComponent)
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
