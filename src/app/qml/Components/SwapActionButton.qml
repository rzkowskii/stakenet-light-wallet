import QtQuick 2.4
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.0

import com.xsn.utils 1.0

Button {
    id: actionButton
    scale: state === "Pressed" ? 0.96 : 1.0
    clip: true

    property string borderColor: ""
    property alias radius: bgRectangle.radius

    contentItem: XSNLabel {
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text: actionButton.text
        font.pixelSize: actionButton.font.pixelSize
        color: mouseArea.containsMouse ? SkinColors.mainText : SkinColors.dexDisclaimerTextColor
    }

    background: Item {

        FadedRectangle {
            id: bgRectangle
            anchors.fill: parent
            opacity: enabled ? 1.0 : 0.4
            activeStateColor: "transparent"
            inactiveStateColor: "transparent"
            activeBorderColor: SkinColors.mainText
            inactiveBorderColor: borderColor
            border.width: 2
            radius: 30
        }
    }

    PointingCursorMouseArea {
        id: mouseArea
        onClicked: {
            actionButton.clicked()
        }

        onExited: {
            actionButton.state = ''
            bgRectangle.stopFade()
            bgRectangle.borderStopFade()
        }
        onEntered: {
            bgRectangle.startFade()
            bgRectangle.borderStartFade()
        }
        onPressed: {
            actionButton.state = "Pressed"
        }
    }
}
