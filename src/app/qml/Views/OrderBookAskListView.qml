import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0

import "../Components"

import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

ListView {
    id: root
    spacing: 3
    boundsBehavior: Flickable.StopAtBounds
    clip: true
    signal clicked(double amount, double price)

    ScrollBar.vertical: ScrollBar {
        size: parent.height / parent.contentHeight
    }

    delegate: DelegateBackgroundItem {
        id: item
        height: 32
        width: root.width

        onSelected: root.clicked(model.amount, model.price)

        RowLayout {
            anchors.fill: parent
            anchors.rightMargin: 15
            anchors.leftMargin: 10
            spacing: 0

            Item {
                Layout.fillHeight: true
                Layout.preferredWidth: parent.width * 0.25

                Image {
                    id: image
                    visible: model.isOwn
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    sourceSize: Qt.size(15, 15)
                    source: "qrc:/images/IC_STAR.png"
                }

                OrderBookText {
                    anchors.right: parent.right
                    anchors.rightMargin: 5
                    anchors.verticalCenter: parent.verticalCenter
                    color: SkinColors.transactionItemSent
                    text: Utils.formatBalance(model.price)
                }
            }

            OrderBookText {
                Layout.fillHeight: true
                Layout.preferredWidth: parent.width * 0.25
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
                text: Utils.formatBalance(model.amount)
            }

                OrderBookText {
                    Layout.fillHeight: true
                    Layout.preferredWidth: parent.width * 0.25
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                    text: Utils.formatBalance(
                              model.amount * Utils.convertSatoshiToCoin(
                                  model.price))
                }

                OrderBookText {
                    Layout.fillHeight: true
                    Layout.preferredWidth: parent.width * 0.25
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                    text: Utils.formatBalance(model.sum)
                }
        }

    }
}
