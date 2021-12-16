import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12

import "../Components"

import com.xsn.utils 1.0

FocusScope {
    id: focusScope
    height: textInput.height + placeholderComposed.implicitHeight
    activeFocusOnTab: true

    property alias placeholderText: textInput.placeholderText
    property alias placeholderTextColor: textInput.placeholderTextColor
    property alias unitTextColor: unitLabel.color
    property alias text: textInput.text
    property alias validator: textInput.validator
    property alias componentFocus: textInput.focus
    property string unitValue: ""
    property alias echoMode: textInput.echoMode
    property alias radius: fadeBackground.radius
    property alias textFieldHeight: textInput.height
    signal accepted()

    Label {
        id: placeholderComposed
        visible: textInput.length > 0
        anchors.left: textInput.left
        anchors.top: textInput.top
        anchors.topMargin: -placeholderComposed.implicitHeight - 3
        anchors.leftMargin: 5
        text: placeholderText
        color: SkinColors.comboboxIndicatorColor
        background: Item {}
        font.family: textInput.font.family
        font.pixelSize: 12
        opacity: focusScope.enabled ? 1 : 0.7
    }

    TextField {
        id: textInput
        height: 41
        anchors.bottom: parent.bottom
        width: parent.width
        selectByMouse: true
        focus: true

        background: FadedRectangle {
            id: fadeBackground
            activeStateColor: SkinColors.headerBackground
            inactiveStateColor: SkinColors.menuBackground
            activeBorderColor: SkinColors.headerText
            inactiveBorderColor: SkinColors.popupFieldBorder
        }

        cursorDelegate: CursorDelegate {
            visible: textInput.cursorVisible
        }

        font.family: regularFont.name
        font.pixelSize: 14
        color: SkinColors.mainText
        placeholderText: ""
        placeholderTextColor: SkinColors.comboboxIndicatorColor
        opacity: focusScope.enabled ? 1 : 0.7

        rightPadding: 15 + unitLabel.contentWidth
        onActiveFocusChanged: {
            if(activeFocus) {
                fadeBackground.startFade()
            }
            else {
                fadeBackground.stopFade();
            }
        }
        onAccepted: focusScope.accepted()

        Text {
            id: unitLabel
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.rightMargin: 10
            text: unitValue
            font.family: regularFont.name
            font.pixelSize: 14
            color: SkinColors.secondaryText
            opacity: focusScope.enabled ? 1 : 0.7
        }

        StyledMouseArea {
            id: styledMouseArea
            anchors.fill: parent

            onClicked: {
                selectStart = textInput.selectionStart;
                selectEnd = textInput.selectionEnd;
                curPos = textInput.cursorPosition;
                contextMenu.x = mouse.x;
                contextMenu.y = mouse.y;
                contextMenu.open();
                textInput.cursorPosition = curPos;
                textInput.select(selectStart,selectEnd);
            }
            onEntered: fadeBackground.borderStartFade();
            onExited: fadeBackground.borderStopFade();
        }

        Menu {
            id: contextMenu

            Action {
                text: qsTr("Copy")
                enabled: textInput.selectedText
                onTriggered: textInput.copy()
            }
            Action {
                text: qsTr("Cut")
                enabled: textInput.selectedText
                onTriggered: textInput.cut()
            }
            Action {
                text: qsTr("Paste")
                enabled: textInput.canPaste
                onTriggered: textInput.paste()
            }

            delegate: MenuItemDelegate {
                id: menuItem
            }

            background: Rectangle {
                implicitWidth: 180
                implicitHeight: 30
                color: SkinColors.menuBackground
                border.color: SkinColors.popupFieldBorder
            }
        }
    }
}
