import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12

import "../Components"

import com.xsn.utils 1.0

ComboBox {
    id: control
    property bool isItem: control.currentIndex !== -1 && currentItem
    property var currentItem: model.get(currentIndex)
    property int currentAssetID: isItem ? currentItem.id : -1
    property string currentSymbol: isItem ? currentItem.symbol : ""

    property int actualIndex: -1
    property int actualAssetID: -1

    Connections {
        target: model
        function onDataChanged() {
            currentItem = model.get(currentIndex)
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

        BotComboBoxItem {
            anchors.fill: parent
            coinName: model.name
            coinSymbol: model.symbol
        }

        background: Rectangle {
            radius: 10
            color: parent.highlighted ? SkinColors.botComboBoxHighlightedColor : SkinColors.botComboBoxBackgroundColor
            opacity: 0.1
        }

        onClicked: {
            actualAssetID = currentAssetID
            actualIndex = currentIndex
        }

        highlighted: control.highlightedIndex === index
    }

    background: Rectangle {
        anchors.fill: parent
        color: "transparent"
    }

    contentItem: StackLayout {
        anchors.fill: parent
        currentIndex: isItem ? 0 : 1

        BotComboBoxItem {
            id: comboBoxItem
            Layout.fillWidth: true
            Layout.fillHeight: true
            coinName: isItem ? currentItem.name : ""
            coinSymbol: isItem ? currentItem.symbol : ""
        }

        Item {
            Layout.fillHeight: true
            Layout.fillWidth: true

            Text {
                anchors.centerIn: parent
                text: "Select"
                color: SkinColors.mainText
                font.pixelSize: 15
                font.family: fontRegular.name
            }
        }
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
        }

        background: Rectangle {
            radius: 10
            color: SkinColors.botComboBoxPopupBackgroundColor
            border.color: SkinColors.botComboBoxPopupBackgroundBorderColor
            border.width: 1
        }
        onClosed: MouseEventSpy.setEventFilerEnabled(true)
        onOpened: MouseEventSpy.setEventFilerEnabled(false)
    }
}
