import QtQuick 2.6
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.3

import com.xsn.utils 1.0

MenuItem {
    id: menuItem
    implicitHeight: 30
    implicitWidth: 180

    FontLoader { id: fontRegular; source: "qrc:/Rubik-Regular.ttf" }
    
    contentItem: Item {
        RowLayout {
            anchors.leftMargin: 7
            anchors.rightMargin: 7
            anchors.fill: parent
            spacing: 15

            Image {
                source: menuItem.icon.source
                sourceSize: Qt.size(menuItem.icon.width, menuItem.icon.height)
            }

            Text {
                leftPadding: menuItem.indicator.width
                rightPadding: menuItem.arrow.width
                text: menuItem.text
                opacity: enabled ? 1.0 : 0.5
                color: menuItem.highlighted ? SkinColors.mainText : SkinColors.secondaryText
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
                font.pixelSize: 14
                font.family: fontRegular.name
            }

            Item {
               Layout.fillWidth: true
            }
        }
    }
    
    background: FadedRectangle {
        id: backGr
        anchors.fill: parent
        anchors.margins: 1
        opacity: enabled ? 1 : 0.5
        activeStateColor: SkinColors.headerBackground
        inactiveStateColor: SkinColors.menuBackground
    }

    PointingCursorMouseArea {
        anchors.fill: parent
        onEntered: backGr.startFade();
        onExited:  backGr.stopFade();
        onClicked: menuItem.action.triggered();
    }
}
