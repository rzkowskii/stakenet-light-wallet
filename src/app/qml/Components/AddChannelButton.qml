import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0

import com.xsn.utils 1.0

import "../Components"

FadedRectangle {
    id: addChannelRec
    activeStateColor: SkinColors.headerBackground
    inactiveStateColor: SkinColors.secondaryBackground
    radius: 32

    property alias text: label.text
    signal clicked()

    FontLoader { id: regularFont; source: "qrc:/Rubik-Regular.ttf" }
    
    Row {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 5
        
        XSNLabel {
            id: label
            font.pixelSize: 11
            color: SkinColors.menuItemText
            anchors.verticalCenter: parent.verticalCenter
            font.family: regularFont.name
        }
        
        Image {
            anchors.verticalCenter: parent.verticalCenter
            source: "qrc:/images/ic_new_channel.svg"
            sourceSize: Qt.size(15, 15)
        }
    }
    
    PointingCursorMouseArea {
        anchors.fill: parent
        
        onClicked: addChannelRec.clicked()
        onEntered: {
            addChannelRec.startFade()
        }
        onExited: {
            addChannelRec.stopFade()
        }
    }
}
