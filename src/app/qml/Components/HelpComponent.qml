import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0

import com.xsn.viewmodels 1.0
import com.xsn.models 1.0
import com.xsn.utils 1.0

import "../Components"
import "../Views"
import "../Popups"

Item {
    property alias toolTip: customToolTip

    width: helpImage.width
    height: helpImage.height

    Image {
        id: helpImage
        source: "qrc:/images/ic_help.png"

        MouseArea {
            id: mouseArea
            hoverEnabled: true
            anchors.fill: parent
        }
    }

    CustomToolTip {
        id: customToolTip
        x: -(customToolTip.width) / 2
        parent: helpImage
        tailPosition: width / 2
        implicitHeight: 41
        implicitWidth: 100
        visible: mouseArea.containsMouse
    }
}
