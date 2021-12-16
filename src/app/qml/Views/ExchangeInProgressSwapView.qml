import QtQuick 2.15
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.15

import "../Views"
import "../Components"
import "../Popups"

import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0
import com.xsn.models 1.0

Item {
    id: root

    property int receiveAssetID: -1

    property string receiveAssetName: ""
    property string sendAssetName: ""

    property string receiveAssetColor: ""
    property string sentAssetColor: ""

    property string receiveAssetSymbol: ""
    property double willReceveAmount: 0

    signal newExchangeButtonClicked

    property alias receiveImage: recImage
    property alias exchangeInProgress: inProgressText
    property alias checkmark: checkMark
    property alias description: descriptionText
    property alias willReceiveLocal: willReceiveLocalText
    property alias willReceiveCoins: willReceiveText

    property alias horizonalSeparatedLine1: hSeparatedLine1
    property alias horizonalSeparatedLine2: hSeparatedLine2
    property alias horizonalSeparatedLine3: hSeparatedLine3

    property alias newExchangeSwapButton: newExchangeButton

    property alias descriptionItem: descriptItem
    property alias inProgresItem: progressItem
    property alias willReceiveItem: willReceive

    FontLoader {
        id: fontRegular
        source: "qrc:/Rubik-Regular.ttf"
    }

    FontLoader {
        id: fontLight
        source: "qrc:/Rubik-Light.ttf"
    }

    GradientBackgroundComponent {
        anchors.fill: parent
        gradientOpacity: 0.7
        firstColor: SkinColors.swapPageBackgroundFooterFirst
        secondColor: SkinColors.swapPageBackgroundFooterSecond
        thirdColor: SkinColors.swapPageBackgroundFooterThird
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 30
        anchors.bottomMargin: 30
        anchors.leftMargin: 30
        anchors.rightMargin: 30
        spacing: root.height * 0.03 //32

        Item {
            Layout.preferredHeight: root.height * 0.23 // 8 225
            Layout.fillWidth: true

            GlowImage {
                id: recImage
                anchors.centerIn: parent
                width: 90
                height: 100
                sourceSize: Qt.size(90, 100)
                radius: 120
                samples: 240
                spread: 0.2
                color: receiveAssetColor !== "" ? receiveAssetColor : ""
                source: receiveAssetName !== "" ? "qrc:/images/ICON_%1.svg".arg(
                                                      receiveAssetName) : ""
            }

            Rectangle {
                id: checkMark
                anchors.horizontalCenter: parent.horizontalCenter
                height: 30
                width: 30
                radius: 15
                anchors.top: recImage.bottom
                anchors.topMargin: -20
                color: "#989dcc"

                Image {
                    anchors.centerIn: parent
                    source: "qrc:/images/IC_CHECK.svg"
                    sourceSize: Qt.size(20, 20)
                }
            }
        }

        Item {
            id: progressItem
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredHeight: root.height * 0.08
            Layout.fillWidth: true

            Text {
                id: inProgressText
                anchors.horizontalCenter: parent.horizontalCenter
                horizontalAlignment: Text.AlignHCenter
                text: "Exchange in progress!"
                font.family: fontLight.name
                font.pixelSize: 32
                color: SkinColors.mainText
            }
        }

        SwapSeparatedLine {
            id: hSeparatedLine1
            Layout.fillWidth: true
            Layout.preferredHeight: 1
        }

        Item {
            id: descriptItem
            Layout.preferredHeight: root.height * 0.08 //108
            Layout.fillWidth: true

            Text {
                id: descriptionText
                lineHeight: 1.1
                anchors.horizontalCenter: parent.horizontalCenter
                horizontalAlignment: Text.AlignHCenter
                text: receiveAssetID >= 0 ? "Please wait for your swap to complete before\nstarting a new exchange": ""
                font.family: fontRegular.name
                font.pixelSize: 16
                color: SkinColors.mainText
            }
        }

        SwapSeparatedLine {
            id: hSeparatedLine2
            Layout.fillWidth: true
            Layout.preferredHeight: 1
        }

        ColumnLayout {
            id: willReceive
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredHeight: root.height * 0.12 // 0.22 108
            Layout.fillWidth: true
            spacing: 12

            Text {
                id: willReceiveLocalText
                horizontalAlignment: Text.AlignHCenter
                font.family: fontRegular.name
                font.pixelSize: 16
                color: SkinColors.mainText
                text: "You will receive %1%2 %3".arg(
                          ApplicationViewModel.localCurrencyViewModel.currentCurrencySymbol).arg(
                          ApplicationViewModel.localCurrencyViewModel.convert(
                              receiveAssetID,
                              Utils.formatBalance(willReceveAmount))).arg(
                          ApplicationViewModel.localCurrencyViewModel.currentCurrencyCode)
            }

            Text {
                id: willReceiveText
                horizontalAlignment: Text.AlignHCenter
                font.family: fontRegular.name
                font.pixelSize: 26
                color: receiveAssetColor
                text: receiveAssetSymbol !== "" ? "+%1 %2".arg(
                                                      Utils.formatBalance(
                                                          willReceveAmount)).arg(
                                                      receiveAssetSymbol) : ""
            }
        }

        SwapSeparatedLine {
            id: hSeparatedLine3
            Layout.fillWidth: true
            Layout.preferredHeight: 1
        }

        GradientButton {
            id: newExchangeButton
            Layout.preferredHeight: root.height / 2 * 0.16
            Layout.alignment: Qt.AlignHCenter
            Layout.maximumHeight: 50
            Layout.preferredWidth: root.width * 0.22
            borderColor: SkinColors.popupFieldBorder
            buttonGradientLeftHoveredColor: sentAssetColor !== "" ? sentAssetColor : "transparent"
            buttonGradientRightHoveredColor: receiveAssetColor
                                             !== "" ? receiveAssetColor : "transparent"
            font.family: fontRegular.name
            font.pixelSize: 16
            enabled: false
            text: "Start new exchange"
            font.capitalization: Font.MixedCase
            radius: 10
            onClicked: newExchangeButtonClicked()
        }
    }
}
