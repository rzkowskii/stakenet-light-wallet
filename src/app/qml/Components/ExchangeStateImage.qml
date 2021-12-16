import QtQuick 2.15
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.15

import com.xsn.utils 1.0

Item {
    property string exchangeState: ""

    Image {
        anchors.horizontalCenter: parent.horizontalCenter
        source: "qrc:/images/wires-base.png"
        sourceSize: Qt.size(450, 1000)
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 10
        opacity: 1.0
    }

    Image {
        anchors.horizontalCenter: parent.horizontalCenter
        source: {
            switch (exchangeState) {
            case "in progress":
                return "qrc:/images/wires-base.png"
            case "success":
                return "qrc:/images/wires-green.png"
            case "failed":
                return "qrc:/images/wires-red.png"
            default:
                return "qrc:/images/wires-base.png"
            }
        }
        sourceSize: Qt.size(450, 1000)
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 10
        opacity: 1.0

        SequentialAnimation on opacity {
            loops: Animation.Infinite
            PropertyAnimation {
                to: 0.0
                duration: 700
            }
            PropertyAnimation {
                to: 1.0
                duration: 700
            }
        }
    }

    Image {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 60
        sourceSize: Qt.size(420, 220)
        source: "qrc:/images/boxes.png"
    }

    Image {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottomMargin: 45
        anchors.bottom: parent.bottom
        sourceSize: Qt.size(290, 160)
        source: "qrc:/images/lines_base.png"
    }

    Image {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottomMargin: 45
        anchors.bottom: parent.bottom
        sourceSize: Qt.size(290, 160)
        source: {
            switch (exchangeState) {
            case "in progress":
                return "qrc:/images/lines_base.png"
            case "success":
                return "qrc:/images/lines_success.png"
            case "failed":
                return "qrc:/images/lines_failed.png"
            default:
                return "qrc:/images/lines_base.png"
            }
        }

        SequentialAnimation on opacity {
            loops: Animation.Infinite
            PropertyAnimation {
                to: 0.0
                duration: 700
            }
            PropertyAnimation {
                to: 1.0
                duration: 700
            }
        }
    }
}
