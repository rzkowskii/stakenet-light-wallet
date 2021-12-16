import QtQuick 2.6
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import "../Components"

import com.xsn.viewmodels 1.0
import com.xsn.models 1.0
import com.xsn.utils 1.0

ActionDialog {
    id: root
    width: 480
    height: 250

    property WalletDexViewModel walletDexViewModel
    property string baseSymbol: ""
    property string quoteSymbol: ""
    property string amount: ""
    property string total: ""
    property string price: ""
    property bool isBuy: false

    signal orderAccepted(string amount, string total, double price, bool isBuy)

    FontLoader { id: regularFont; source: "qrc:/Rubik-Regular.ttf" }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 30
        spacing: 25

        Text {
            Layout.fillWidth: true
            Layout.preferredHeight: 35
            color: SkinColors.mainText
            lineHeight: 1.5
            text: "You are %1 %2 %3 in exchange for %4 %5" .arg(isBuy ? "buying" : "selling").arg(amount).arg(baseSymbol).arg(total).arg(quoteSymbol)
            wrapMode: Text.WordWrap
            font.family: regularFont.name
            font.pixelSize: 18
            horizontalAlignment: Text.AlignHCenter
        }

        RowLayout {
            Layout.preferredHeight: 20
            Layout.fillWidth: true
            Layout.leftMargin: 20
            spacing: 5

            CheckBox {
                id: checkbox
                checked: false

                indicator: Rectangle {
                    implicitWidth: 17
                    implicitHeight: 17
                    x: checkbox.leftPadding
                    y: parent.height / 2 - height / 2
                    radius: 6
                    border.color: SkinColors.mainText
                    color: "transparent"
                    border.width: 1


                    ColorOverlayImage {
                        width: 15
                        height: 15
                        x: 2
                        y: 1
                        imageSize: 15
                        color: SkinColors.mainText
                        imageSource: "qrc:/images/checkmark-round.png"
                        visible: checkbox.checked
                    }
                }

                PointingCursorMouseArea {
                    onClicked:  {
                       checkbox.checked = !checkbox.checked
                    }

                    onEntered:  {
                        checkbox.scale = scale * 1.1
                    }
                    onExited: {
                        checkbox.scale = scale
                    }
                }
            }

            Text {
                font.weight: Font.Thin
                text: "Ignore this box in the future"
                font.family: regularFont.name
                font.pixelSize: 12
                color: SkinColors.mainText

            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            spacing: 25

            IntroButton {
                Layout.preferredHeight: 40
                Layout.preferredWidth: 150
                font.pixelSize: 12
                text: "Cancel"
                onClicked: {
                    close()
                }
            }

            IntroButton {
                id: confirmButton
                Layout.preferredHeight: 40
                Layout.preferredWidth: 150
                font.pixelSize: 12
                text: "Ok"
                onClicked: {
                    if(checkbox.checked)
                    {
                        walletDexViewModel.changePlaceOrderBoxVisibility();
                    }

                    orderAccepted(amount, total, Utils.parseCoinsToSatoshi(price), isBuy);
                    enabled = false
                    root.close()
                }
            }
        }
    }
}
