import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0
import QtQuick.Controls 2.2

import "../Components"

import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

ListView {
    id: root
    Layout.fillHeight: true
    Layout.fillWidth: true

    signal balanceVisibilityChanged

    signal sendCoinsRequested(int id)
    signal receiveCoinsRequested(int id)

    property bool balanceVisible

    boundsBehavior: Flickable.StopAtBounds
    clip: true
    delegate: Rectangle {
        id: delegate
        height: closedTransactionHeight
        color: SkinColors.secondaryBackground
        radius: 4
        width: parent.width

        Connections {
            target: ApplicationViewModel.localCurrencyViewModel
            function onCurrencyRateChanged(assetID) {
                if (delegate.assetID === assetID) {
                    localBalance.refresh()
                }
            }
        }

        Connections {
            target: root
            function onBalanceVisibilityChanged() {
                localBalance.refresh()
            }
        }

        onBalanceOnChainChanged: {
            localBalance.refresh()
        }
        onNodeBalanceChanged: {
            localBalance.refresh()
        }
        onTotalBalanceChanged: {
            localBalance.refresh()
        }

        property int assetID: model.id
        property string name: model.name
        property string currency: model.symbol
        property var totalBalance: Utils.formatBalance(model.balance)
        property var balanceOnChain: Utils.formatBalance(model.balanceOnChain)
        property var nodeBalance: Utils.formatBalance(model.nodeBalance)
        property double portfolioPercent: model.percent.toFixed(2)

        signal balanceVisibleChanged

        Component.onCompleted: {
            localBalance.refresh()
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.rightMargin: 20

            Row {
                Layout.preferredWidth: parent.width * 0.35
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignVCenter
                spacing: 10

                Image {
                    anchors.verticalCenter: parent.verticalCenter
                    source: "qrc:/images/ICON_%1.svg".arg(name)
                    sourceSize: Qt.size(35, 40)
                }

                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 5

                    XSNLabel {
                        font.pixelSize: 14
                        text: name
                    }

                    SecondaryLabel {
                        text: currency
                        font.capitalization: Font.AllUppercase
                    }
                }
            }

            ColumnLayout {
                Layout.preferredWidth: parent.width * 0.27
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                spacing: 5

                RowLayout {
                    Layout.rightMargin: 10
                    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                    spacing: 0

                    XSNLabel {
                        font.pixelSize: 14
                        text: "%1 %2".arg(
                                  balanceVisible ? balanceOnChain : hideBalance(
                                                       balanceOnChain)).arg(
                                  currency)
                    }

                    XSNLabel {
                        font.pixelSize: 14
                        color: SkinColors.secondaryText
                        text: "  ("
                    }

                    Image {
                        source: "qrc:/images/ic_lightning_small.png"
                    }

                    SecondaryLabel {
                        font.pixelSize: 14
                        text: "%1 %2)".arg(
                                  balanceVisible ? nodeBalance : hideBalance(
                                                       nodeBalance)).arg(
                                  currency)
                    }
                }

                SecondaryLabel {
                    id: localBalance
                    Layout.rightMargin: 10
                    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                    function refresh() {
                        text = "%1 %2".arg(
                                    balanceVisible ? ApplicationViewModel.localCurrencyViewModel.convert(
                                                         model.id,
                                                         totalBalance) : hideBalance(
                                                         ApplicationViewModel.localCurrencyViewModel.convert(
                                                             model.id,
                                                             totalBalance))).arg(
                                    ApplicationViewModel.localCurrencyViewModel.currentCurrencyCode)
                    }
                }
            }

            Item {
                Layout.preferredWidth: parent.width * 0.2
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignVCenter

                SecondaryLabel {
                    text: "%1 %".arg(portfolioPercent)
                    anchors.leftMargin: 30
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignHCenter
                spacing: 7

                SecondaryLabel {
                    text: "Send"
                    color: SkinColors.menuItemText

                    AnimationColor on color {
                        id: animationSendActive
                        propAnimation.to: SkinColors.mainText
                    }

                    AnimationColor on color {
                        id: animationSendInactive
                        propAnimation.to: SkinColors.menuItemText
                    }

                    PointingCursorMouseArea {
                        onClicked: sendCoinsRequested(assetID)
                        onEntered: animationSendActive.start()
                        onExited: animationSendInactive.start()
                    }
                }

                SecondaryLabel {
                    text: "Receive"
                    color: SkinColors.menuItemText
                    AnimationColor on color {
                        id: animationReceiveActive
                        propAnimation.to: SkinColors.mainText
                    }

                    AnimationColor on color {
                        id: animationReceiveInactive
                        propAnimation.to: SkinColors.menuItemText
                    }

                    PointingCursorMouseArea {
                        onClicked: receiveCoinsRequested(assetID)
                        onEntered: animationReceiveActive.start()
                        onExited: animationReceiveInactive.start()
                    }
                }
            }
        }
    }

    focus: true
    spacing: 5

    add: Transition {
        NumberAnimation {
            properties: "y"
            from: root.height
            duration: 200
        }
    }
    addDisplaced: Transition {
        NumberAnimation {
            properties: "x,y"
            duration: 200
        }
    }
}
