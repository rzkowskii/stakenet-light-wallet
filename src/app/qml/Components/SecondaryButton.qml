import QtQuick 2.4
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.0

import com.xsn.utils 1.0

Button {
    id: actionButton
    scale: state === "Pressed" ? 0.96 : 1.0
    property string borderColor: ""
    property string hoveredBorderColor: ""
    property string activeStateColor: "transparent"


    Behavior on scale {
        NumberAnimation {
            duration: 100
            easing.type: Easing.InOutQuad
        }
    }

    contentItem: XSNLabel {
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text: actionButton.text
        font.capitalization: actionButton.font.capitalization
        font.pixelSize: actionButton.font.pixelSize
    }

    background: FadedRectangle {
        id: bgRectangle
        anchors.fill: parent
        activeStateColor: actionButton.activeStateColor
        activeBorderColor: hoveredBorderColor
        inactiveBorderColor: borderColor
        inactiveStateColor: "transparent"
        border.width: 2
        radius: 30
        opacity: 0.5
    }

    PointingCursorMouseArea {
        onClicked: actionButton.clicked()
        onExited: {
            bgRectangle.stopFade();
            bgRectangle.borderStopFade()
            actionButton.state = ''
        }
        onPressed: {
            actionButton.state = "Pressed"
        }
        onEntered: {
            bgRectangle.startFade();
            bgRectangle.borderStartFade();
        }
    }
}
