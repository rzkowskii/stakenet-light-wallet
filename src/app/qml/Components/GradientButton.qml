import QtQuick 2.4
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.0

import com.xsn.utils 1.0

Button {
    id: actionButton
    scale: state === "Pressed" ? 0.96 : 1.0
    clip: true

    property string buttonGradientLeftHoveredColor: SkinColors.introBtnGradientHoveredColor
    property string buttonGradientLeftColor: SkinColors.introBtnGradientColor

    property string buttonGradientRightHoveredColor: SkinColors.introBtnGradientHoveredColor
    property string buttonGradientRightColor: SkinColors.introBtnGradientColor

    property string borderColor: ""
    property alias radius: bgRectangle.radius

    Behavior on scale {
        NumberAnimation {
            duration: 200
            easing.type: Easing.InOutQuad
        }
    }

    contentItem: XSNLabel {
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text: actionButton.text
        font.pixelSize: actionButton.font.pixelSize
        color: enabled ? SkinColors.mainText : SkinColors.secondaryText
    }

    background: FadedRectangle {
        id: bgRectangle
        opacity: enabled ? 1.0 : 0.4
        activeStateColor: SkinColors.menuBackground
        inactiveStateColor: "transparent"
        activeBorderColor: "transparent"
        inactiveBorderColor: borderColor
        border.width: 2
        radius: 30

        LinearGradient {
            source: bgRectangle
            anchors.fill: parent
            start: Qt.point(0, 0)
            end: Qt.point(width, 0)
            opacity: mouseArea.containsMouse ? 1 : 0.7
            gradient: Gradient {
                id: bgGradient

                GradientStop {
                    position: 1.0
                    color: buttonGradientRightHoveredColor
                }
                GradientStop {
                    id: grad
                    position: 0.0
                    color: buttonGradientLeftHoveredColor
                }
            }
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
