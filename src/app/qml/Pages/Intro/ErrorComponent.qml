import QtQuick 2.4
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2

import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

import Components 1.0

StackViewPage {
    skipButton.visible: false
    headerText: "STAKENET | DEX"
    headerSource: SkinColors.mainWalletLogo
    backButton.visible: false

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
            Layout.preferredHeight: 553
            Layout.alignment: Qt.AlignVCenter
            color: "#99151B30"

            ColumnLayout {
                anchors.fill: parent
                spacing: 20
                anchors.bottomMargin: 60

                Image {
                    source: "qrc:/images/crossIcon.png"
                    sourceSize: Qt.size(267, 259)
                    Layout.alignment: Qt.AlignHCenter
                }

                XSNLabel {
                    Layout.alignment: Qt.AlignHCenter
                    text: "Wallet Created"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 22
                    font.bold: true
                }

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: 274
                    text: "Your 24 word seed phrase didn`t match. Please try again!"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    wrapMode: Text.WordWrap
                    font.pixelSize: 15
                    color: "#7377A5"
                }

                IntroButton {
                    Layout.preferredWidth: 275
                    Layout.preferredHeight: 48
                    Layout.topMargin: 15
                    Layout.alignment: Qt.AlignHCenter
                    radius: 8
                    text: "Continue"
                    textColor: "white"
                    borderColor: "transparent"
                    buttonColor: "#1254DD"
                    buttonGradientColor: "#1D96EC"
                    buttonHoveredColor: "#1254DD"
                    buttonGradientHoveredColor: "#1D96EC"
                }
            }

            PointingCursorMouseArea {
                onClicked: {
                    stackView.pop()
                    stackView.pop()
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}
