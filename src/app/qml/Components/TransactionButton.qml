import QtQuick 2.12
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.0


Button {
    id: actionButton

    property color baseColor: "transparent"
    property color secondaryColor: "transparent"
    property int radius: 0

    background: Rectangle {
        color: actionButton.baseColor
        radius: actionButton.radius
        gradient: Gradient {
            GradientStop { position: 0.0; color: baseColor }
            GradientStop { position: 1.0; color: Qt.darker(baseColor) }
        }

        PointingCursorMouseArea {

            onClicked: actionButton.clicked()
            onEntered:  {
                actionButton.scale = scale * 1.05
            }
            onExited: {
                actionButton.scale = scale
            }
        }
    }
}
