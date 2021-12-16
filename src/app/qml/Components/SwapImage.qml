import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.15

import "../Views"
import "../Components"
import "../Popups"

import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0
import com.xsn.models 1.0

Image {
    id: root
    property alias rightArrowShadowColor: shadowRight.color
    property alias leftArrowShadowColor: shadowLeft.color
    property alias swapArrowsItem: item

    signal clicked

    source: "qrc:/images/ICONOGRAPHY-SWAP.svg"

    RotationAnimation {
        id: rotationAnimation
        target: item
        duration: 500
        from: 0
        to: 180
    }

    Item {
        id: item
        width: parent.width / 2
        height: parent.height / 2
        anchors.centerIn: parent

        Image {
            id: rightArrow
            anchors.right: parent.right
            anchors.rightMargin: parent.width * 0.1
            anchors.verticalCenterOffset: -parent.height * 0.08
            anchors.verticalCenter: parent.verticalCenter
            sourceSize: Qt.size(parent.width * 0.53, parent.height * 0.28)
            source: "qrc:/images/arrow_white_right.svg"
        }

        DropShadow {
            id: shadowRight
            anchors.fill: rightArrow
            horizontalOffset: 0
            verticalOffset: rightArrow.height * 0.1
            radius: 1
            samples: 25
            source: rightArrow
            spread: 0
        }

        Glow {
            anchors.fill: rightArrow
            radius: 45
            spread: 0.1
            color: "white"
            samples: 91
            source: rightArrow
        }

        Image {
            id: leftArrow
            anchors.left: parent.left
            anchors.leftMargin: parent.width * 0.1
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: parent.height * 0.08
            sourceSize: Qt.size(parent.width * 0.53, parent.height * 0.28)
            source: "qrc:/images/arrow_white_left.svg"
        }

        DropShadow {
            id: shadowLeft
            anchors.fill: leftArrow
            horizontalOffset: 0
            verticalOffset: leftArrow.height * 0.1
            radius: 0
            samples: 25
            source: leftArrow
            spread: 0
        }

        Glow {
            anchors.fill: leftArrow
            radius: 45
            spread: 0.1
            color: "white"
            samples: 91
            source: leftArrow
        }

        PointingCursorMouseArea {
            id: mouseArea
            anchors.fill: parent
            onClicked: {
                root.clicked()
                //rotationAnimation.start()
            }
        }
    }
}
