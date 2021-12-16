import QtQuick 2.15
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0
import QtQuick.Controls.Styles 1.4

import "../Components"
import "../Popups"

import com.xsn.models 1.0
import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

Item {
    id: root

    signal sendCoinsRequested
    signal receiveCoinsRequested
    signal refreshRequested
    signal openInExplorerRequested(string txid)
    signal apChannelCapacityRequested

    FontLoader {
        id: fontRegular
        source: "qrc:/Rubik-Regular.ttf"
    }
    FontLoader {
        id: fontLight
        source: "qrc:/Rubik-Light.ttf"
    }

    property int coinID: 0
    property bool balanceVisible
    property LocalCurrencyViewModel localCurrencyViewModel: ApplicationViewModel.localCurrencyViewModel
    property PaymentNodeViewModel paymentNodeViewModel
    property WalletAssetViewModel walletViewModel
    property var currentAssetNodeStatus

    DexRefundableFeeModel {
        id: refundingModel
        currentAssetID: coinID
        Component.onCompleted: {
            initialize(ApplicationViewModel)
        }
    }

    Component {
        id: preAPOpenChannelPopupComponent
        PreAPOpenChannelPopup {}
    }

    Component {
        id: openChannelComponent
        SliderOpenChannelPopup {

            onOpenChannelCanceled: {
                slider.refreshSliderBalance()
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: parent.height * 0.5
            Layout.leftMargin: 42
            Layout.rightMargin: 42
            Layout.topMargin: 32
            Layout.bottomMargin: 32

            spacing: 32

            FadedImage {
                Layout.preferredHeight: 85
                Layout.preferredWidth: 85
                imageSize: Qt.size(40, 46)
                imageSource: mouseArea.containsMouse ? "qrc:/images/ic_refresh.svg" : walletViewModel.assetInfo.name !== undefined ? "qrc:/images/ICON_%1.svg".arg(walletViewModel.assetInfo.name) : ""
                fadeColor: walletViewModel.assetInfo.color

                PointingCursorMouseArea {
                    id: mouseArea
                    onClicked: {
                        refreshRequested()
                    }
                }
            }

            ColumnLayout {
                Layout.maximumWidth: 100
                XSNLabel {
                    Layout.alignment: Qt.AlignLeft
                    text: "BALANCE"
                    font.pixelSize: 15
                    font.family: fontRegular.name
                    color: SkinColors.mainText
                }

                XSNLabel {
                    Layout.alignment: Qt.AlignLeft
                    text: "%1%2".arg(
                              ApplicationViewModel.localCurrencyViewModel.currentCurrencySymbol).arg(
                              balanceVisible ? ApplicationViewModel.localCurrencyViewModel.convert(
                                                   coinID, Utils.formatBalance(
                                                       walletViewModel.assetBalance.total)) : hideBalance(
                                                   ApplicationViewModel.localCurrencyViewModel.convert(
                                                       coinID,
                                                       Utils.formatBalance(
                                                           walletViewModel.assetBalance.total))))
                    font.pixelSize: 32
                    font.family: fontRegular.name
                    color: SkinColors.mainText
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 20
                    clip: false

                    XSNLabel {
                        text: "%1 %2".arg(
                                  balanceVisible ? Utils.formatBalance(
                                                       walletViewModel.assetBalance.total) : hideBalance(
                                                       Utils.formatBalance(
                                                           walletViewModel.assetBalance.total))).arg(
                                  walletViewModel.assetInfo.symbol)
                        font.pixelSize: 15
                        color: SkinColors.mainText
                    }

                    XSNLabel {
                        text: "1 %1 = %2 %3".arg(
                                  walletViewModel.assetInfo.symbol).arg(
                                  ApplicationViewModel.localCurrencyViewModel.convert(
                                      coinID, "1")).arg(
                                  ApplicationViewModel.localCurrencyViewModel.currentCurrencySymbol)
                        font.pixelSize: 15
                        color: SkinColors.mainText
                    }
                }
            }

            Item {
                Layout.fillWidth: true
            }

            RowLayout {
                Layout.alignment: Qt.AlignTop
                spacing: 16

                ActionButton {
                    Layout.preferredWidth: 120
                    Layout.preferredHeight: 40

                    background: Rectangle {
                        anchors.fill: parent
                        radius: 10
                        gradient: Gradient {
                            GradientStop {
                                position: 0.0
                                color: "#0c1021"
                            }
                            GradientStop {
                                position: 1.0
                                color: SkinColors.delegatesBackgroundDarkColor
                            }
                        }

                        RowLayout {
                            anchors {
                                fill: parent
                                rightMargin: 12
                                leftMargin: 16
                            }
                            spacing: 20

                            Text {
                                Layout.fillWidth: true
                                horizontalAlignment: Text.AlignLeft
                                text: qsTr("Receive")
                                font.pixelSize: 16
                                font.family: fontRegular.name
                                color: SkinColors.mainText
                            }

                            Image {
                                Layout.alignment: Qt.AlignVCenter
                                source: "qrc:/images/icon-receive.svg"
                                sourceSize: Qt.size(20, 20)
                                rotation: -90
                            }
                        }
                    }

                    onClicked: receiveCoinsRequested()
                }

                ActionButton {
                    id: sendBtn
                    Layout.preferredWidth: 120
                    Layout.preferredHeight: 40

                    background: Rectangle {
                        anchors.fill: parent
                        radius: 10
                        gradient: Gradient {
                            GradientStop {
                                position: 0.0
                                color: SkinColors.delegatesBackgroundLightColor
                            }
                            GradientStop {
                                position: 1.0
                                color: SkinColors.delegatesBackgroundDarkColor
                            }
                        }

                        RowLayout {
                            anchors {
                                fill: parent
                                rightMargin: 12
                                leftMargin: 16
                            }
                            spacing: 20

                            Text {
                                Layout.fillWidth: true
                                horizontalAlignment: Text.AlignLeft
                                text: qsTr("Send")
                                font.pixelSize: 16
                                font.family: fontRegular.name
                                color: SkinColors.mainText
                            }

                            Image {
                                Layout.alignment: Qt.AlignVCenter
                                source: "qrc:/images/icon-send.svg"
                                sourceSize: Qt.size(20, 20)
                                rotation: -90
                            }
                        }
                    }

                    onClicked: sendCoinsRequested()
                }
            }
        }

        RoundedRectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: parent.height * 0.5

            corners.bottomLeftRadius: 10
            corners.bottomRightRadius: 10
            customGradient: {
                "vertical": false,
                "colors": [{
                               "position": 0.0,
                               "color": SkinColors.walletPageHeaderViewBlueColor
                           }, {
                               "position": 0.5,
                               "color": SkinColors.walletPageBackgroundLightColor
                           }, {
                               "position": 1.0,
                               "color": SkinColors.walletPageHeaderViewBlueColor
                           }]
            }

            ColumnLayout {
                anchors {
                    fill: parent
                    topMargin: 15
                    leftMargin: 40
                    rightMargin: 40
                    bottomMargin: 18
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    RowLayout {
                        spacing: 20
                        Rectangle {
                            Layout.preferredHeight: 37
                            Layout.preferredWidth: 37

                            color: "#1ad8d8d8"
                            radius: 10

                            Image {
                                anchors.centerIn: parent
                                source: "qrc:/images/icon-chain-on.svg"
                                sourceSize: Qt.size(18, 18)
                            }
                        }

                        ColumnLayout {
                            Layout.fillHeight: true
                            spacing: 0
                            XSNLabel {
                                Layout.alignment: Qt.AlignLeft
                                text: qsTr("ON CHAIN")
                                font.pixelSize: 10
                                color: SkinColors.mainText
                            }

                            XSNLabel {
                                text: "%1 %2".arg(
                                          balanceVisible ? Utils.formatBalance(
                                                               walletViewModel.assetBalance.onChain) : hideBalance(Utils.formatBalance(walletViewModel.assetBalance.onChain))).arg(
                                          walletViewModel.assetInfo.symbol)
                                font.pixelSize: 15
                                color: SkinColors.mainText
                            }
                        }

                        XSNLabel {
                            Layout.alignment: Qt.AlignBottom
                            text: "%1 %2".arg(
                                      ApplicationViewModel.localCurrencyViewModel.currentCurrencySymbol).arg(
                                      ApplicationViewModel.localCurrencyViewModel.convert(
                                          coinID, Utils.formatBalance(
                                              walletViewModel.assetBalance.onChain)))
                            font.pixelSize: 11
                            color: SkinColors.mainText
                            opacity: 0.5
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    RowLayout {
                        spacing: 20

                        XSNLabel {
                            Layout.alignment: Qt.AlignBottom
                            text: "%1 %2".arg(
                                      ApplicationViewModel.localCurrencyViewModel.currentCurrencySymbol).arg(
                                      ApplicationViewModel.localCurrencyViewModel.convert(
                                          coinID, Utils.formatBalance(
                                              paymentNodeViewModel.stateModel.nodeBalance)))

                            font.pixelSize: 11
                            color: SkinColors.mainText
                            opacity: 0.5
                        }

                        ColumnLayout {
                            Layout.fillHeight: true
                            spacing: 0
                            XSNLabel {
                                Layout.alignment: Qt.AlignRight
                                text: qsTr("OFF CHAIN")
                                font.pixelSize: 10
                                color: SkinColors.mainText
                            }

                            XSNLabel {
                                text: "%1 %2".arg(
                                          balanceVisible ? Utils.formatBalance(
                                                               paymentNodeViewModel.stateModel.nodeBalance) : hideBalance(Utils.formatBalance(paymentNodeViewModel.stateModel.nodeBalance))).arg(
                                          walletViewModel.assetInfo.symbol)
                                font.pixelSize: 15
                                color: SkinColors.mainText
                            }
                        }

                        Rectangle {
                            Layout.preferredHeight: 37
                            Layout.preferredWidth: 37

                            color: "#1ad8d8d8"
                            radius: 10

                            Image {
                                anchors.centerIn: parent
                                source: "qrc:/images/icon-chain-off.svg"
                                sourceSize: Qt.size(18, 18)
                            }
                        }
                    }

                    NodeStatusIndicator {
                        Layout.alignment: Qt.AlignTop
                        Layout.preferredHeight: 8
                        Layout.preferredWidth: 8
                        payNodeViewModel: paymentNodeViewModel
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 30
                    radius: 12
                    color: "#33070c19"

                    PaymentNodeSlider {
                        id: slider
                        anchors.fill: parent
                        payNodeViewModel: paymentNodeViewModel
                        assetID: coinID
                        assetName: walletViewModel.assetInfo.name
                        assetSymbol: walletViewModel.assetInfo.symbol
                        assetConfirmationsForApproved: walletViewModel.assetInfo.сonfirmationsForApproved
                        assetMinPayNodeCapacity: (payNodeViewModel.type === Enums.PaymentNodeType.Lnd) ? walletViewModel.assetInfo.minLndCapacity : 0.1
                        assetTotalBalance: walletViewModel.assetBalance.total
                        assetOnChainBalance: walletViewModel.assetBalance.onChain
                        assetNodeBalance: paymentNodeViewModel.stateModel.nodeBalance
                        from: 0
                        to: 100
                        value: calculateBalancePercentage(
                                   paymentNodeViewModel.stateModel.nodeBalance,
                                   walletViewModel.assetBalance.total, 1)
                        lastValue: calculateBalancePercentage(
                                       paymentNodeViewModel.stateModel.nodeBalance,
                                       walletViewModel.assetBalance.total, 1)
                        valueSymbol: "%"
                        stepSize: 0.1
                    }
                }

                RowLayout {
                    Layout.fillWidth: true

                    RowLayout {
                        Layout.preferredWidth: 50

                        spacing: 5

                        FadedText {
                            id: dexRefundsLabel
                            text: "DEX Refunds: "
                            font.pixelSize: 12
                            horizontalAlignment: Text.AlignLeft
                            verticalAlignment: Text.AlignVCenter
                            color: SkinColors.secondaryText
                            font.family: fontRegular.name
                            font.capitalization: Font.MixedCase

                            MouseArea {
                                id: dexRefundsLabelMouseArea
                                anchors.fill: parent
                                hoverEnabled: true
                                onEntered: dexRefundsLabel.startFade()
                                onExited: dexRefundsLabel.stopFade()
                            }
                        }

                        XSNLabel {
                            text: "%1 %2".arg(
                                      ApplicationViewModel.localCurrencyViewModel.convert(
                                          coinID, Utils.formatBalance(
                                              refundingModel.refudanbleAmount))).arg(
                                      ApplicationViewModel.localCurrencyViewModel.currentCurrencyCode)
                            font.pixelSize: 12
                            horizontalAlignment: Text.AlignLeft
                            verticalAlignment: Text.AlignVCenter
                            font.family: fontRegular.name

                            MouseArea {
                                id: dexRefundsMouseArea
                                anchors.fill: parent
                                hoverEnabled: true
                                onEntered: dexRefundsLabel.startFade()
                                onExited: dexRefundsLabel.stopFade()
                            }
                        }

                        CustomToolTip {
                            height: 41
                            width: 280
                            x: -20
                            parent: dexRefundsLabel
                            visible: dexRefundsMouseArea.containsMouse
                                     || dexRefundsLabelMouseArea.containsMouse
                            tailPosition: 60
                            text: "Fee refunds from cancelled/failed trades"
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    GradientButton {
                        id: applyButton
                        Layout.preferredHeight: 35
                        Layout.preferredWidth: 120
                        Layout.alignment: Qt.AlignHCenter
                        borderColor: SkinColors.popupFieldBorder
                        buttonGradientLeftHoveredColor: "#ffc27c"
                        buttonGradientRightHoveredColor: SkinColors.walletAssetHighlightColor
                        font.family: fontRegular.name
                        font.pixelSize: 15
                        text: "Apply"
                        radius: 10

                        onClicked: {
                            if (checkPaymentNodeState(
                                        payNodeViewModel.stateModel)) {
                                openDialog(openChannelComponent, {
                                               "assetID": coinID,
                                               "assetName": walletViewModel.assetInfo.name,
                                               "assetSymbol": walletViewModel.assetInfo.symbol,
                                               "confirmationsForApproved": walletViewModel.assetInfo.сonfirmationsForApproved,
                                               "channelCapacity": slider.channCapacity,
                                               "paymentNodeViewModel": payNodeViewModel
                                           })
                            }
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    XSNLabel {
                        Layout.preferredWidth: 20
                        text: "%1%".arg(slider.value.toFixed(1))
                        font.pixelSize: 12
                        color: SkinColors.mainText
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        Layout.leftMargin: 5
                    }
                }
            }
        }
    }

    function calculateBalancePercentage(lightningBalance, totalBalance, precision) {
        var percentages = Number.isNaN(
                    (lightningBalance / totalBalance)
                    * 100) ? 0 : (lightningBalance / totalBalance) * 100
        var num = Number(percentages)
        var roundedString = num.toFixed(precision)
        var rounded = Number(roundedString)
        return rounded
    }
}
