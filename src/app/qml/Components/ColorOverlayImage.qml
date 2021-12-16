import QtQuick 2.12
import QtGraphicalEffects 1.0

Item {
    property string imageSource: ""
    property string color: ""
    property int imageSize: 0

    Image {
        id: image
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
        source: imageSource
        sourceSize: Qt.size(imageSize, imageSize)
    }

    ColorOverlay {
        anchors.fill: image
        source: image
        color: parent.color
    }
}
