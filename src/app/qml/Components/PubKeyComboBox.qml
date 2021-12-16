import QtQuick 2.6
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.0

import "../Components"

import com.xsn.utils 1.0

ComboBox {
    id: control
    
    property alias comboBoxBackground: backGrd
    property alias contentSearch : content
    
    textRole: "display"
    font.pixelSize: 14
    
    delegate: ItemDelegate {
        width: control.width
        
        contentItem: Text {
            anchors.left: parent.left
            anchors.leftMargin: 15
            text: "HUB Node %1".arg(model.index + 1)
            font.pixelSize: 14
            font.family: fontRegular.name
            color: parent.highlighted ? SkinColors.mainText : SkinColors.secondaryText
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
        
        background: Rectangle {
            color: parent.highlighted ? SkinColors.headerBackground : SkinColors.menuBackground
            opacity: 0.5
        }
        
        onClicked: {
            content.text = model.display
        }
        
        highlighted: control.highlightedIndex === index
    }
    
    contentItem: StyledTextInput {
        id: content
        anchors.fill: parent
        onActiveFocusChanged: {
            if(activeFocus && !popup.opened)
            {
                popup.open()
                return;
            }
            if(!activeFocus)
            {
                popup.close()
            }
       }
    }
    
    background: Rectangle {
        id: backGrd
        implicitWidth: 200
        implicitHeight: 35
        radius: 2
        color: SkinColors.menuBackground
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
        
        onClosed: content.focus = false
        onOpened: MouseEventSpy.setEventFilerEnabled(false)
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
