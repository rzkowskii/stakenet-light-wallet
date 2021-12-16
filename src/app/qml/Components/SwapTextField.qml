import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12

import "../Components"

import com.xsn.utils 1.0
import com.xsn.viewmodels 1.0

FocusScope {
    id: focusScope
    activeFocusOnTab: true

    property alias placeholderText: textInput.placeholderText
    property alias placeholderTextColor: textInput.placeholderTextColor
    property alias text: textInput.text
    property alias validator: textInput.validator
    property alias componentFocus: textInput.focus
    property string unitValue: ""
    property bool symbolOnLeft: false

    FontLoader {
        id: fontRegular
        source: "qrc:/Rubik-Regular.ttf"
    }

    TextField {
        id: textInput
        anchors.fill: parent
        selectByMouse: true
        focus: true

        background: FadedRectangle {
            id: fadeBackground
            activeStateColor: SkinColors.headerBackground
            inactiveStateColor: SkinColors.swapPageSecondaryBackground
            activeBorderColor: SkinColors.headerText
            inactiveBorderColor: SkinColors.swapComboBoxBackground
            border.width: 1
            radius: 10
        }

        cursorDelegate: CursorDelegate {
            visible: textInput.cursorVisible
        }
        font.family: fontRegular.name
        font.pixelSize: 16
        color: SkinColors.mainText
        placeholderTextColor: "#212637"
        opacity: focusScope.enabled ? 1 : 0.7
        leftPadding: symbolOnLeft ? 15 + unitLabel.contentWidth : 25

        rightPadding: symbolOnLeft ? 15 : 25 + unitLabel.contentWidth
        onActiveFocusChanged: {
            if(activeFocus) {
                fadeBackground.startFade()
            }
            else {
                fadeBackground.stopFade();
            }
        }

        Text {
            id: unitLabel
            anchors.left: symbolOnLeft ?  parent.left : undefined
            anchors.leftMargin: symbolOnLeft ? 10 : 0
            anchors.right: symbolOnLeft ? undefined : parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.rightMargin: symbolOnLeft ? 0 : 10
            text: unitValue
            font.family: fontRegular.name
            font.pixelSize: 15
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
