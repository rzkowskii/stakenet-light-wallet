import QtQuick 2.6
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12
import QtGraphicalEffects 1.0

import com.xsn.utils 1.0
import com.xsn.models 1.0
import com.xsn.viewmodels 1.0

FocusScope {
    id: focusScope
    activeFocusOnTab: true
    property alias componentFocus: textEdit.focus
    property alias text: textEdit.text
    property alias cursorVisible: textEdit.cursorVisible
    property alias editText: textEdit
    
    FadedRectangle {
        id: fadeBackground
        anchors.fill: parent
        activeStateColor: SkinColors.headerBackground
        inactiveStateColor: SkinColors.menuBackground
        activeBorderColor: SkinColors.headerText
        inactiveBorderColor: SkinColors.popupFieldBorder
        border.width: 1
        radius: 10
        
        TextEdit {
            id: textEdit
            anchors.fill: parent
            verticalAlignment: Text.AlignVCenter
            font.family: fontRegular.name
            font.pixelSize: 14
            color: SkinColors.mainText
            leftPadding: 15
            selectByMouse: true
            focus: false
            clip: true
            
            Text {
                id: placeholderText
                anchors.verticalCenter: parent.verticalCenter
                text: "Search pair for swap"
                font.family: fontRegular.name
                font.pixelSize: 14
                leftPadding: 15
                color: SkinColors.comboboxIndicatorColor
                visible: textEdit.text == ""
            }
            
            cursorDelegate: CursorDelegate {
                visible: textEdit.cursorVisible
            }

            onActiveFocusChanged: {
                if(activeFocus) {
                    fadeBackground.startFade()
                }
                else {
                    fadeBackground.stopFade();
                }
            }
            
            StyledMouseArea {
                id: styledMouseArea
                anchors.fill: parent
                onEntered: fadeBackground.borderStartFade();
                onExited: fadeBackground.borderStopFade();
            }
        }
    }
}
