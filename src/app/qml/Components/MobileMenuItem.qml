import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0

import com.xsn.utils 1.0

import "../Components"

Item {
    id: root
    signal menuItemClicked()
    property bool isCurrentItem: false
    property string name: ""
    property string imageSource: ""

    Image {
        anchors.centerIn: parent
        source: root.imageSource
        sourceSize: Qt.size(35, 35)
    }

    PointingCursorMouseArea {
        id: mouseArea
        onClicked: menuItemClicked()
    }
}
