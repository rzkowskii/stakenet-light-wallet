import QtQuick 2.12
import QtQuick.Controls 2.2

import com.xsn.utils 1.0

Button {
    id: actionButton
    property string source: ""
    property color color: "transparent"
    property int radius: 0

    background: Rectangle {
        color: mouseArea.containsMouse ? actionButton.color : "transparent"
        radius: actionButton.radius
        opacity: 0.15

        PointingCursorMouseArea {
            id: mouseArea

            onClicked: actionButton.clicked()
            onEntered:  {
                actionButton.scale = scale * 1.1
            }
            onExited: {
                actionButton.scale = scale
            }
        }
    }

    contentItem: ColorOverlayImage {
        anchors.centerIn: parent
        imageSize: 23
        imageSource: actionButton.source
        color: SkinColors.settingsShareIcons
    }
}
