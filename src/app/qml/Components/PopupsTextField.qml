import QtQuick 2.1
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2

import QtGraphicalEffects 1.0

TextField {
    id: textField
    font.pixelSize: 14
    readOnly: true
    background: Rectangle {
        color: "transparent"
    }
    selectionColor: "#323856"

    LinearGradient {
        visible: selectedText === ""
        anchors.fill: parent
        source: textField
        start: Qt.point(width, 0)
        end: Qt.point(0, 0)
        gradient: Gradient {
            GradientStop { position: 1.0; color: "#8C9CD4" }
            GradientStop { position: 0.5; color: "#454C71" }
            GradientStop { position: 0.0; color: "#171C30" }
        }
    }
}
