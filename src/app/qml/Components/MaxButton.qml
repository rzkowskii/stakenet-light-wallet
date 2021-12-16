import QtQuick 2.6
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtQuick.Controls 2.2 as NewControls
import QtGraphicalEffects 1.0
import "../Components"

import com.xsn.viewmodels 1.0
import com.xsn.models 1.0
import com.xsn.utils 1.0

Rectangle {
    id: root
    color: "transparent"
    border.color: maxMouseArea.containsMouse ? SkinColors.headerText : SkinColors.popupFieldBorder
    border.width: 1
    signal clicked()

    FontLoader { id: regularFont; source: "qrc:/Rubik-Regular.ttf" }
    
    Button {
        anchors.fill: parent
        background: Rectangle {
            color: "transparent"
        }
        
        XSNLabel {
            anchors.centerIn: parent
            font.family: regularFont.name
            font.pixelSize: 14
            text: "MAX"
        }
        
        PointingCursorMouseArea {
            id: maxMouseArea
            onClicked: root.clicked()
        }
    }
}
