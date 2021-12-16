import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0

import com.xsn.utils 1.0

import "../Components"

FadedRectangle {
    id: channelRentalRec
    activeStateColor: SkinColors.headerBackground
    inactiveStateColor: SkinColors.secondaryBackground
    radius: 32

    signal clicked()

    FontLoader { id: regularFont; source: "qrc:/Rubik-Regular.ttf" }

    Row {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 5
               
        XSNLabel {
            font.pixelSize: 11
            color: SkinColors.menuItemText
            anchors.verticalCenter: parent.verticalCenter
            text: "Channel Rental"
            font.family: regularFont.name
        }
        
        Image {
            anchors.verticalCenter: parent.verticalCenter
            source: "qrc:/images/ic_channel_rental.svg"
            sourceSize: Qt.size(15, 15)
        }
    }
    
    PointingCursorMouseArea {
        anchors.fill: parent
        onClicked: channelRentalRec.clicked()
        onEntered: {
            channelRentalRec.startFade()
        }
        onExited: {
            channelRentalRec.stopFade()
        }
    }
}
