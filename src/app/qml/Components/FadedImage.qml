import QtQuick 2.12
import QtGraphicalEffects 1.0


Item {
    property color fadeColor
    property string imageSource
    property alias imageSize: image.sourceSize


    Rectangle {
        anchors.fill: parent
        radius: 10
        opacity: 0.2
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 1.0; color: fadeColor }
            GradientStop { position: 0.0; color: "transparent" }
        }
    }

    Image {
        id: image
        anchors.centerIn: parent
        source: imageSource
    }
}
