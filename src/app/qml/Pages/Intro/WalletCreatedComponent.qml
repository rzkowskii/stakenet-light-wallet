import QtQuick 2.4
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2

import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

import Components 1.0

StackViewPage {
    id: welcomePage
    skipButton.visible: false
    headerText: "STAKENET | DEX"
    headerSource: SkinColors.mainWalletLogo
    property bool isOpenRescanNotification: false
    property string mainHeaderText: ""
    property string secondaryHeaderText: ""
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
            Layout.preferredHeight: 553
            Layout.alignment: Qt.AlignVCenter
            color: "#99151B30"

            ColumnLayout {
                anchors.fill: parent
                spacing: 20
                anchors.bottomMargin: 60

                Image {
                    source: "qrc:/images/wallet_completed.svg"
                    sourceSize: Qt.size(267, 259)
                    Layout.alignment: Qt.AlignHCenter
                }

                XSNLabel {
                    id: mainHeaderLbl
                    Layout.alignment: Qt.AlignHCenter
//                    text: "Wallet Created"
                    text: mainHeaderText
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 22
                    font.bold: true
                }

                Text {
                    id: secondaryHeaderLbl
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: 274
//                    text: "Your wallet has been successfully created. Thank you for using Stakenet DEX!"
                    text: secondaryHeaderText
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
                    replaceView(mainPage, {isOpenRescanNotificationPopup : isOpenRescanNotification})
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}
