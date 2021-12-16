import QtQuick 2.12
import QtQuick.Controls 2.2

Item {
    id: closeButton
    signal clicked()

    Image {
        anchors.right: parent.right
        anchors.top: parent.top
        source: "qrc:/images/ic_close.svg"
        sourceSize: Qt.size(30, 30)
    }

    PointingCursorMouseArea {
        onEntered: closeButton.scale = 1.05
        onExited: closeButton.scale = 1.0
        onClicked: closeButton.clicked()
    }
}
