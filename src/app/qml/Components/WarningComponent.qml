import QtQuick 2.6
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.0
import "../Components"

import com.xsn.viewmodels 1.0
import com.xsn.models 1.0
import com.xsn.utils 1.0

Item {
    implicitWidth: 400
    implicitHeight: 50

    property alias message: messageText.text

    Image {
        anchors.fill: parent
        source: "qrc:/images/BG NOTIF.png"
    }
    
    RowLayout {
        anchors.fill: parent
        
        Item {
            Layout.fillHeight: true
            Layout.preferredWidth: 65
            
            Image {
                anchors.centerIn: parent
                source: "qrc:/images/ICON WARN.png"
                sourceSize: Qt.size(40, 40)
            }
        }
        
        XSNLabel {
            id: messageText
            Layout.leftMargin: 5
            Layout.rightMargin: 10
            Layout.fillWidth: true
            font.family: regularFont.name
            font.pixelSize: 14
            wrapMode: Text.WordWrap
            color: SkinColors.mainText
        }
    }
    
}
