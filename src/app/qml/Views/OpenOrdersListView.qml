import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0

import "../Components"

import com.xsn.models 1.0
import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

ListView {
    id: listView
    spacing: 3
    property WalletDexViewModel dexViewModel
    property var popup
    clip: true
    boundsBehavior: Flickable.StopAtBounds

    model:  dexViewModel.stateModel.ownOrderBookListModel
    delegate: DelegateBackgroundItem {
        height: 35
        width: listView.width

        RowLayout {
            anchors.rightMargin: 15
            anchors.leftMargin: 15
            anchors.fill: parent
            spacing: 0

            OrderBookText {
                Layout.preferredWidth: 0.1 * parent.width
                text: "Limit"
                font.capitalization: Font.MixedCase
                horizontalAlignment: Text.AlignHCenter
            }

            OrderBookText {
                text: model.side === Enums.OrderSide.Buy ? "Buy" : "Sell"
                Layout.preferredWidth: 0.1 * parent.width
                color: model.side === Enums.OrderSide.Buy ? "#3FBE4D" : "#E2344F"
                font.capitalization: Font.MixedCase
                horizontalAlignment: Text.AlignHCenter
            }

            OrderBookText {
                text: Utils.formatBalance(model.price)
                Layout.preferredWidth: 0.1 * parent.width
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
            }

            OrderBookText {
                Layout.preferredWidth: 0.2 * parent.width
                //                text: Utils.formatBalance(model.open)
                text: Utils.formatBalance(model.amount)
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
            }

            OrderBookText {
                Layout.preferredWidth: 0.2 * parent.width
                text: "%1 % (%2)".arg(model.filledPercentage.toFixed(2)).arg(
                          Utils.formatBalance(model.completed))
                horizontalAlignment: Text.AlignHCenter
            }

            OrderBookText {
                Layout.preferredWidth: 0.2 * parent.width
                //                    text: Utils.formatBalance(model.open * Utils.convertSatoshiToCoin(model.price))
                text: Utils.formatBalance(
                          model.amount * Utils.convertSatoshiToCoin(
                              model.price))
                horizontalAlignment: Text.AlignHCenter
            }

            OrderBookText {
                Layout.preferredWidth: 0.1 * parent.width
                text: "Cancel"
                font.capitalization: Font.AllUppercase
                color: "#E2344F"
                horizontalAlignment: Text.AlignHCenter

                PointingCursorMouseArea {
                    onClicked: {
                        popup = openConfirmRemoveOrderDialog({
                                                                 "message": "Are you sure you want to cancel order?"
                                                             })
                        popup.confirmClicked.connect(cancelOrder)
                    }
                }
            }
        }

        function cancelOrder() {
            dexViewModel.cancelOrder(model.id)
            popup.close()
        }
    }
}
