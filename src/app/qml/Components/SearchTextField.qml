import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12

import "../Components"

import com.xsn.utils 1.0

FocusScope {
    id: focusScope
    activeFocusOnTab: true

    property alias placeholderText: searchArea.placeholderText
    property alias componentFocus: searchArea.focus
    property alias text: searchArea.text
    property alias componentActiveFocus: searchArea.activeFocus
    property alias validator: searchArea.validator

    TextField {
        id: searchArea
        FontLoader {
            id: regularFont
            source: "qrc:/Rubik-Regular.ttf"
        }
        anchors.fill: parent
        font.pixelSize: 15
        placeholderTextColor: SkinColors.secondaryText
        font.family: regularFont.name
        color: SkinColors.mainText
        selectByMouse: true
        focus: true
        background: Rectangle {
            color: "transparent"
        }

        cursorDelegate: CursorDelegate {
            visible: searchArea.cursorVisible
        }

        onSelectedTextChanged: {
            mouseArea.selectStart = searchArea.selectionStart
            mouseArea.selectEnd = searchArea.selectionEnd
        }

        onCursorPositionChanged: {
            mouseArea.selectStart = 0
            mouseArea.selectEnd = 0
        }

        StyledMouseArea {
            id: mouseArea
            anchors.fill: parent

            onClicked: {
                curPos = searchArea.cursorPosition
                contextMenu.x = mouse.x
                contextMenu.y = mouse.y
                contextMenu.open()
                searchArea.cursorPosition = curPos
                searchArea.select(selectStart, selectEnd)
            }
        }

        Menu {
            id: contextMenu

            Action {
                text: qsTr("Copy")
                enabled: searchArea.selectedText != ""
                onTriggered: searchArea.copy()
            }
            Action {
                text: qsTr("Cut")
                enabled: searchArea.selectedText != ""
                onTriggered: searchArea.cut()
            }
            Action {
                text: qsTr("Paste")
                enabled: searchArea.canPaste
                onTriggered: searchArea.paste()
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
