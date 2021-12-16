import QtQuick 2.6
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.0

import com.xsn.utils 1.0

ComboBox {
    id: control
    property var currentItem: model.get(currentIndex)
    property double currentMinLndCapacity: currentItem.minLndCapacity
    property double currentMaxLndCapacity: currentItem.maxLndCapacity
    property int currentAssetID: currentItem.id
    property string currentSymbol: currentItem.symbol
    property double currentBalanceOnChain: currentItem.balanceOnChain
    property double currentConfirmedBalanceOnChain: currentItem.confirmedBalanceOnChain
    property double currentLndBalance: currentItem.nodeBalance
    property bool showOnlyLndBalance: false
    property double currentActiveLndBalance: currentItem.activeLndBalance
    property double currentAverageTxFee: currentItem.averageTxFee

    Connections {
        target: sortedAssetModel
        function onDataChanged() {
            currentItem = model.get(currentIndex)
        }
    }

    Layout.fillWidth: true

    delegate: ItemDelegate {
        width: control.width
        height: control.height

        CoinsComboBoxItem {
            coinName: model.name
            coinAmount: Utils.formatBalance(
                            showOnlyLndBalance ? model.activeLndBalance : model.confirmedBalanceOnChain)
            coinValue: model.symbol
            border.color: SkinColors.mainText
        }

        highlighted: control.highlightedIndex === index
    }

    contentItem: CoinsComboBoxItem {
        id: comboBoxItem
        anchors.fill: parent
        coinName: currentItem.name
        coinAmount: Utils.formatBalance(
                        showOnlyLndBalance ? currentItem.activeLndBalance : currentItem.confirmedBalanceOnChain)
        coinValue: currentItem.symbol
        border.color: SkinColors.mainBackground
    }

    indicator: Canvas {
        id: canvas
        x: control.width - width - control.rightPadding
        y: control.topPadding + (control.availableHeight - height) / 2
        width: 12
        height: 8
        contextType: "2d"

        Connections {
            target: control
            function onPressedChanged() {
                canvas.requestPaint()
            }
        }

        onPaint: {
            context.reset()
            context.moveTo(0, 0)
            context.lineTo(width, 0)
            context.lineTo(width / 2, height)
            context.closePath()
            context.fillStyle = "#8692C3"
            context.fill()
        }
    }

    popup: Popup {
        y: control.height - 1
        width: control.width
        implicitHeight: contentItem.implicitHeight
        padding: 0

        contentItem: ListView {
            implicitHeight: contentHeight
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex
            boundsBehavior: Flickable.StopAtBounds

            Rectangle {
                anchors.fill: parent
                color: "transparent"
                border.color: SkinColors.mainText
                border.width: 2
            }
        }
        onClosed: MouseEventSpy.setEventFilerEnabled(true)
        onOpened: MouseEventSpy.setEventFilerEnabled(false)
    }
}
