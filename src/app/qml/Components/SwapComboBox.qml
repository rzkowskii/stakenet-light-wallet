import QtQuick 2.15
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.0

import com.xsn.utils 1.0

ComboBox {
    id: control

    property var currentItem: model.get(currentIndex)
    property bool isItem: control.currentIndex !== -1 && currentItem
    property int currentAssetID: isItem ? currentItem.id : -1
    property string currentSymbol: isItem ? currentItem.symbol : ""
    property double currentTotalBalance: isItem ? currentItem.confirmedBalanceOnChain
                                                  + currentItem.activeLndBalance : 0
    property string currentName: isItem ? currentItem.name : ""
    property string currentColor: isItem ? currentItem.color : ""
    property double currentMinLndCapacity: isItem ? currentItem.minLndCapacity : 0

    property int actualIndex: -1
    property int actualAssetID: -1

    Connections {
        target: model
        function onDataChanged() {
            if (currentIndex < model.count) {
                currentItem = model.get(currentIndex)
            }
        }
    }

    onCurrentIndexChanged: {
        currentItem = model.get(currentIndex)
    }

    onModelChanged: {
        currentItem = model.get(currentIndex)
    }

    delegate: ItemDelegate {
        width: control.width
        height: control.height

        SwapComboBoxItem {
            anchors.fill: parent
            coinName: model.name
            coinAmount: balanceVisible ? Utils.formatBalance(
                                             model.confirmedBalanceOnChain
                                             + model.activeLndBalance) : hideBalance(
                                             Utils.formatBalance(
                                                 model.confirmedBalanceOnChain
                                                 + model.activeLndBalance))
            coinValue: model.symbol
        }

        highlighted: control.highlightedIndex === index

        background: Rectangle {
            color: parent.highlighted ? SkinColors.headerBackground : SkinColors.secondaryBackground
            opacity: 0.5
        }
        onClicked: {
            actualAssetID = currentAssetID
            actualIndex = currentIndex
        }
    }

    contentItem: SwapComboBoxItem {
        id: comboBoxItem
        anchors.fill: parent
        defaultItem: control.currentIndex === -1
        coinName: isItem ? currentItem.name : ""
        coinAmount: isItem ? mainPage.balanceVisible ? Utils.formatBalance(
                                                           currentTotalBalance) : hideBalance(
                                                           Utils.formatBalance(
                                                               currentTotalBalance)) : ""
        coinValue: isItem ? currentItem.symbol : ""
    }

    background: Rectangle {
        id: backGrd
        radius: 10
        color: SkinColors.swapComboBoxBackground
        opacity: 0.8
    }

    indicator: Image {
        width: 25
        height: 25
        x: control.width - width - control.rightPadding
        y: control.topPadding + (control.availableHeight - height) / 2
        source: control.popup.visible ? "qrc:/images/ic_up.svg" : "qrc:/images/ic_down.svg"
    }

    popup: Popup {
        y: control.height - 1
        width: control.width
        implicitHeight: contentItem.implicitHeight
        padding: 0

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex
            boundsBehavior: Flickable.StopAtBounds
        }

        background: Rectangle {
            radius: 10
            color: SkinColors.headerBackground
            border.color: SkinColors.comboboxIndicatorColor
        }
        onClosed: comboBoxItem.focus = false
        onOpened: MouseEventSpy.setEventFilerEnabled(false)
    }
}
