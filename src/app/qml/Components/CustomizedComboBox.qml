import QtQuick 2.12
import QtQuick.Controls 2.1

import com.xsn.utils 1.0

ComboBox {
    id: control
    property alias content: textItem

    font.pixelSize: 14

    delegate: ItemDelegate {
        width: control.width
        contentItem: XSNLabel {
            text: modelData
            color: SkinColors.headerText
            font: control.font
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: isMobile ? Text.AlignHCenter : Text.AlignLeft
            anchors.left: parent.left
            anchors.leftMargin: 15
        }

        background: Rectangle {
            color: parent.highlighted ? SkinColors.headerBackground : SkinColors.mainBackground
            opacity: 0.5
        }

        highlighted: control.highlightedIndex === index
    }

    contentItem: XSNLabel {
        id: textItem
        text: control.displayText
        font: control.font
        color:  SkinColors.mainText
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: isMobile ? Text.AlignHCenter : Text.AlignLeft
        elide: Text.ElideRight
        anchors.left: parent.left
        anchors.leftMargin: 15
    }

    background: Rectangle {
        implicitWidth: 90
        implicitHeight: 30
        radius: 2
        color: SkinColors.mainBackground
    }

    popup: Popup {
        y: control.height - 1
        width: control.width
        implicitHeight: contentItem.implicitHeight > 200 ? (isMobile ? 165 : 200) : contentItem.implicitHeight // 200 - maximumHeight for list
        padding: 1

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex
            boundsBehavior: Flickable.StopAtBounds
        }

        background: Rectangle {
            radius: 2
            color: isMobile ? SkinColors.mobileSecondaryBackground : SkinColors.headerBackground
        }
    }

    indicator: Canvas {
        id: canvas
        x: control.width - width - control.rightPadding
        y: control.topPadding + (control.availableHeight - height) / 2
        width: 10
        height: 7
        contextType: "2d"

        Connections {
            target: control
            function onPressedChanged() {
                canvas.requestPaint()
            }
        }

        onPaint: {
            context.reset();
            context.moveTo(0, 0);
            context.lineTo(width, 0);
            context.lineTo(width / 2, height);
            context.closePath();
            context.fillStyle = control.pressed ? SkinColors.mainText : SkinColors.comboboxIndicatorColor;
            context.fill();
        }
    }
}

