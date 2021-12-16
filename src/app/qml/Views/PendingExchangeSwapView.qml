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

    property string exchangeSt: ""
    property string swapErrorMsg: ""

    property int receiveAssetID: -1
    property string receiveAssetName: ""
    property string receiveAssetSymbol: ""
    property string receiveAssetColor: ""
    property double willReceveAmount: 0

    property string currentState: ""

    property alias horizonalSeparatedLine1: hSeparatedLine1
    property alias horizonalSeparatedLine2: hSeparatedLine2

    property alias checkmark: checkMark
    property alias receiveImage: recImage
    property alias description: descriptionText
    property alias stateText: stText
    property alias willReceiveLocal: willReceiveLocalText
    property alias willReceiveCoins: willReceiveText
    property alias willReceiveComp: willReceive
    property alias info: infoItem
    property alias designedImage: designedImg

    property alias stateItem: stItem

    property alias errorMessageText: errorMsg
    property alias errorItem: errorIt

    property alias cancelImage: cancelImg
    property alias receiveAssetImage: receiveAssetImg

    signal closeButtonClicked

    FontLoader {
        id: fontRegular
        source: "qrc:/Rubik-Regular.ttf"
    }

    FontLoader {
        id: fontLight
        source: "qrc:/Rubik-Light.ttf"
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillHeight: true
            Layout.fillWidth: true
            color: SkinColors.botTextFieldActiveBorderColor

            Image {
                id: cancelImg
                anchors.left: parent.left
                anchors.leftMargin: 32
                anchors.top: parent.top
                anchors.topMargin: 32
                source: "qrc:/images/cross.svg"
                sourceSize: Qt.size(24, 24)
                width: 24
                height: 24
                visible: exchangeSt === "failed" || exchangeSt === "success"

                PointingCursorMouseArea {
                    anchors.fill: parent
                    onClicked: closeButtonClicked()
                }
            }

            ColumnLayout {
                anchors.fill: parent
                spacing: 5

                Item {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                }

                Item {
                    id: stItem
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredHeight: root.height * 0.08
                    Layout.fillWidth: true

                    Text {
                        id: stText
                        anchors.horizontalCenter: parent.horizontalCenter
                        horizontalAlignment: Text.AlignHCenter
                        text: currentState
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
                    Layout.preferredHeight: root.height * 0.08
                    Layout.fillWidth: true
                    visible: exchangeSt === "in progress"

                    Text {
                        id: descriptionText
                        lineHeight: 1.1
                        anchors.horizontalCenter: parent.horizontalCenter
                        horizontalAlignment: Text.AlignHCenter
                        text: receiveAssetID >= 0 ? "We are opening the required channels and your swap will complete\nwithin 30 minutes or less. Please keep your wallet open and online." : ""
                        font.family: fontRegular.name
                        font.pixelSize: 13
                        color: SkinColors.mainText
                        opacity: 0.0
                        visible: true
                    }
                }

                SwapSeparatedLine {
                    id: hSeparatedLine2
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    visible: exchangeSt === "in progress"
                }

                Item {
                    id: errorIt
                    Layout.fillWidth: true
                    Layout.preferredHeight: root.height * 0.1
                    visible: exchangeSt === "failed"
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20

                    Text {
                        id: errorMsg
                        wrapMode: Text.WordWrap
                        width: parent.width
                        anchors.topMargin: 10
                        anchors.horizontalCenter: parent.horizontalCenter
                        horizontalAlignment: Text.AlignHCenter
                        font.family: fontRegular.name
                        font.pixelSize: 15
                        color: "#EC6D75"
                        text: swapErrorMsg
                        opacity: 0.0
                    }
                }

                ColumnLayout {
                    id: willReceive
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredHeight: root.height * 0.12 // 0.22 108
                    Layout.fillWidth: true
                    spacing: 12
                    visible: exchangeSt === "in progress"
                             || exchangeSt === "success"

                    Text {
                        id: willReceiveLocalText
                        horizontalAlignment: Text.AlignHCenter
                        font.family: fontRegular.name
                        Layout.fillWidth: true
                        font.pixelSize: 18
                        color: SkinColors.mainText
                        text: "%1 %2 %3 %4".arg(
                                  exchangeSt
                                  === "in progress" ? "You will receive" : exchangeSt
                                                      === "success" ? "You received" : "").arg(
                                  ApplicationViewModel.localCurrencyViewModel.currentCurrencySymbol).arg(
                                  ApplicationViewModel.localCurrencyViewModel.convert(
                                      receiveAssetID, Utils.formatBalance(
                                          willReceveAmount))).arg(
                                  ApplicationViewModel.localCurrencyViewModel.currentCurrencyCode)
                        opacity: 0.0
                    }

                    Text {
                        id: willReceiveText
                        horizontalAlignment: Text.AlignHCenter
                        Layout.fillWidth: true
                        font.family: fontRegular.name
                        font.pixelSize: 20
                        color: receiveAssetColor
                        font.bold: true
                        text: receiveAssetSymbol !== "" ? "+%1 %2".arg(
                                                              Utils.formatBalance(
                                                                  willReceveAmount)).arg(
                                                              receiveAssetSymbol) : ""
                        opacity: 0.0
                    }
                }

                GradientButton {
                    id: applyButton
                    Layout.preferredHeight: 35
                    Layout.preferredWidth: 120
                    Layout.topMargin: 30
                    Layout.alignment: Qt.AlignHCenter
                    borderColor: SkinColors.popupFieldBorder
                    buttonGradientLeftHoveredColor: "#ffc27c"
                    buttonGradientRightHoveredColor: SkinColors.walletAssetHighlightColor
                    font.family: fontRegular.name
                    font.pixelSize: 15
                    text: "New Trade"
                    radius: 10
                    visible: exchangeSt === "success"
                    onClicked: closeButtonClicked()
                }

                Item {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                }

                RowLayout {
                    id: infoItem
                    Layout.fillWidth: true
                    Layout.topMargin: 30
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    Layout.bottomMargin: 30
                    spacing: 20
                    opacity: 0.0
                    visible: exchangeSt === "in progress"

                    Item {
                        Layout.alignment: Qt.AlignVCenter
                        Layout.preferredHeight: 20
                        Layout.preferredWidth: 20

                        Image {
                            anchors.centerIn: parent
                            source: "qrc:/images/ICON WARN.png"
                            sourceSize: Qt.size(20, 20)
                        }
                    }

                    Text {
                        Layout.alignment: Qt.AlignVCenter
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        font.family: fontRegular.name
                        font.pixelSize: 12
                        color: SkinColors.secondaryText

                        text: qsTr("We are creating the state channels to execute this trade off-chain. When you receive your coins, these will be on layer 2 where you can enjoy the benefits of instant transactions with low fees as well as greater privacy.")
                    }
                }
            }
        }

        Item {
            id: rightRect
            Layout.fillHeight: true
            Layout.fillWidth: true

            GradientBackgroundComponent {
                anchors.fill: parent
                gradientOpacity: 0.5
                firstColor: "#273D6A" //to do
                secondColor: SkinColors.swapPageBackgroundFooterSecond
                thirdColor: SkinColors.swapPageBackgroundFooterThird
            }

            Item {
                anchors.centerIn: parent
                width: 500
                height: 300

                Item {
                    anchors.fill: parent

                    GlowImage {
                        id: recImage
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: 115
                        height: 130
                        sourceSize: Qt.size(115, 130)
                        radius: 150
                        samples: 300
                        spread: 0.2
                        color: receiveAssetColor !== "" ? receiveAssetColor : ""
                        source: {
                            switch (exchangeSt) {
                            case "in progress":
                                return "qrc:/images/path_in_progress.svg"
                            case "success":
                                return "qrc:/images/path_success.svg"
                            case "failed":
                                return "qrc:/images/path_red.svg"
                            default:
                                return "qrc:/images/path_in_progress.svg"
                            }
                        }
                        scale: 0.0
                        opacity: 1.0

                        SequentialAnimation on opacity {
                            loops: Animation.Infinite
                            PropertyAnimation {
                                to: 0.0
                                duration: 700
                            }
                            PropertyAnimation {
                                to: 1.0
                                duration: 700
                            }
                        }
                    }

                    Image {
                        id: receiveAssetImg
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.topMargin: 25
                        anchors.top: parent.top
                        source: receiveAssetName !== "" ? "qrc:/images/ICON_%1.svg".arg(
                                                              receiveAssetName) : ""
                    }

                    Rectangle {
                        id: checkMark
                        anchors.horizontalCenter: parent.horizontalCenter
                        height: 25
                        width: 25
                        radius: 12
                        anchors.top: recImage.bottom
                        anchors.topMargin: -40
                        color: {
                            switch (exchangeSt) {
                            case "in progress":
                                return "#989dcc"
                            case "success":
                                return "#57ff5b"
                            case "failed":
                                return "#ff6868"
                            default:
                                return "#989dcc"
                            }
                        }
                        scale: 0.0

                        Image {
                            anchors.centerIn: parent
                            source: {
                                switch (exchangeSt) {
                                case "in progress":
                                    return "qrc:/images/IC_CHECK.svg"
                                case "success":
                                    return "qrc:/images/IC_CHECK.svg"
                                case "failed":
                                    return "qrc:/images/IC_CROSS.svg"
                                default:
                                    return "qrc:/images/IC_CHECK.svg"
                                }
                            }
                            sourceSize: Qt.size(15, 15)
                        }
                    }

                    ExchangeStateImage {
                        id: designedImg
                        exchangeState: exchangeSt
                        width: 420
                        height: 400
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.top: checkMark.bottom
                        anchors.topMargin: -185
                        opacity: 0.0
                    }
                }
            }
        }
    }
}
