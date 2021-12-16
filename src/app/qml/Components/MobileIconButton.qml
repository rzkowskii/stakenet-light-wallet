import QtQuick 2.12

Rectangle {
    id: root
    property alias image: imageItem
    signal clicked()

    width: 49
    height: 49
    radius: 24

    Image {
        id: imageItem
        anchors.centerIn: parent
    }

    PointingCursorMouseArea {
        onClicked: root.clicked()
    }
}
