import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0

import com.xsn.utils 1.0

Item {
    property alias mouseArea: mouse
    signal selected()

    FadedRectangle {
        id: background
        anchors.fill: parent
        activeStateColor: "#1F1F31"
        inactiveStateColor: "transparent"
        color: "transparent"
    }

    LinearGradient {
        anchors.fill: parent
        opacity: 0.3

        start: Qt.point(0, 0)
        end: Qt.point(0, height)
        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: "transparent"
            }
            GradientStop {
                position: 1.0
                color: "black"
            }
        }
    }

    PointingCursorMouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        onClicked: selected()
        onEntered: background.startFade()
        onExited: background.stopFade()
    }
}
