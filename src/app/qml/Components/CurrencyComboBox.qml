import QtQuick 2.12
import QtQuick.Controls 2.1

import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

ComboBox {
    id: control
    property string currentCode: model.getCode(control.currentIndex)
    property alias comboBoxBackground: backGrd
    property alias content : content
    textRole: "name"
    font.pixelSize: 14

    delegate: ItemDelegate {
        width: control.width
        contentItem: XSNLabel {
            text: "%1 - %2".arg(model.code).arg(model.name)
            font.bold: true
            color: parent.highlighted ? SkinColors.mainText : SkinColors.secondaryText
            font.pixelSize: 14
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }

        background: Rectangle {
            color: parent.highlighted ? SkinColors.headerBackground : SkinColors.mainBackground
            opacity: 0.5
        }

        highlighted: control.highlightedIndex === index
    }

    contentItem: XSNLabel {
        id: content
        text: "  %1 - %2" .arg(control.currentCode) .arg(control.displayText)
        font.bold: true
        color:  SkinColors.mainText
        font.pixelSize: 14
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
        anchors.left: parent.left
        anchors.leftMargin: 7
    }

    background: Rectangle {
        id: backGrd
        implicitWidth: 200
        implicitHeight: 30
        radius: 2
        color: SkinColors.mainBackground
        border.color: SkinColors.comboboxIndicatorColor
        opacity: 0.5
    }

    popup: Popup {
        y: control.height - 1
        width: control.width
        implicitHeight: contentItem.implicitHeight
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
            color: SkinColors.headerBackground
            border.color: SkinColors.comboboxIndicatorColor
        }

        onOpened: MouseEventSpy.setEventFilerEnabled(false)
        onClosed: MouseEventSpy.setEventFilerEnabled(true)
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
            function onPressedChanged(){
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

