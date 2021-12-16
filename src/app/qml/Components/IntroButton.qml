import QtQuick 2.4
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.0

import com.xsn.utils 1.0

Button {
    id: actionButton
    scale: state === "Pressed" ? 0.96 : 1.0

    property string buttonHoveredColor: SkinColors.introBtnHoveredColor
    property string buttonColor: SkinColors.introBtnColor

    property string buttonGradientHoveredColor: SkinColors.introBtnGradientHoveredColor
    property string buttonGradientColor: SkinColors.introBtnGradientColor
    property string borderColor: SkinColors.buttonBorderColor
    property alias radius: bgRectangle.radius
    property alias textColor: contentLabel.color

    Behavior on scale {
        NumberAnimation {
            duration: 100
            easing.type: Easing.InOutQuad
        }
    }

    contentItem: XSNLabel {
        id: contentLabel
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text: actionButton.text
        font.capitalization: Font.AllUppercase
        font.pixelSize: actionButton.font.pixelSize
        opacity: enabled ? 1.0 : 0.4
        color: SkinColors.mainText
    }

    background: Item {

        FadedRectangle {
            id: bgRectangle
            anchors.fill: parent
            opacity: enabled ? 1.0 : 0.4
            activeStateColor: buttonHoveredColor
            inactiveStateColor: buttonColor
            border.color: borderColor
            border.width: 2
            radius: 30

            LinearGradient {
                source: bgRectangle
                anchors.fill: parent
                start: Qt.point(0, 0)
                end: Qt.point(width, 0)
                opacity: 0.4
                gradient: Gradient {
                    id: bgGradient
                    GradientStop { position: 1.0; color: "transparent" }
                    GradientStop { position: 0.6; color: "transparent" }
                    GradientStop {
                        id: grad
                        position: 0.0
                        color: buttonGradientColor

                        AnimationColor on color {
                            id: animationGradientActive
                            propAnimation.to: buttonGradientHoveredColor;
                        }

                        AnimationColor on color {
                            id: animationGradientInactive
                            propAnimation.to: buttonGradientColor
                        }
                    }
                }
            }
        }
    }

    PointingCursorMouseArea {
        onClicked: { actionButton.clicked() }
        onEntered: {
            bgRectangle.startFade();
            animationGradientActive.start();
        }

        onExited: {
            actionButton.state = '';
            bgRectangle.stopFade()
            animationGradientInactive.start();
        }
        onPressed: { actionButton.state = "Pressed" }
    }
}
