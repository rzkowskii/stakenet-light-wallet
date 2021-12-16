import QtQuick 2.12
import QtGraphicalEffects 1.0

import com.xsn.utils 1.0

Item {
    id: root
    signal listHeaderClicked()
    property bool highlighted: true
    property string sortRole: ""

    property string headerText: ""

    implicitWidth: parent.width
    implicitHeight: parent.height

    Rectangle {
        id: background
        color: "transparent"
        anchors.fill: parent

        LinearGradient {
            anchors.fill: parent
            opacity: 0.3
            start: Qt.point(0, 0)
            end: Qt.point(0, height)
            gradient: Gradient {
                GradientStop { position: 0.0; color: "transparent" }
                GradientStop {
                    id: grad
                    position: 1.0
                    color: highlighted ? SkinColors.menuBackgroundGradientFirst : "transparent";

                    AnimationColor on color {
                        id: animationGradientActive
                        propAnimation.to: SkinColors.highlightedItemHeader;
                    }

                    AnimationColor on color {
                        id: animationGradientInactive
                        propAnimation.to: "transparent"
                    }
                }
            }
        }

        FadedRectangle {
            id: fadedRectangle
            activeStateColor: SkinColors.highlightedItemHeader
            inactiveStateColor: "transparent"
            anchors.bottom: parent.bottom;
            width: parent.width
            height: 2
            color: highlighted ? SkinColors.menuBackgroundGradientFirst : "transparent"
        }
    }

    FadedText {
        id: fadedText
        anchors.fill: parent
        anchors.leftMargin: 10
        font.capitalization: Font.Capitalize
        color: highlighted ? SkinColors.mainText : SkinColors.headerText;
        font.pixelSize: 11
        text: headerText
    }

    PointingCursorMouseArea {
        id: mouseArea

        onClicked: {
            listHeaderClicked();
        }

        onEntered:  {
            if(!highlighted)
            {
                animationGradientActive.start();
                animationGradientInactive.stop();
                fadedRectangle.startFade()
                fadedText.startFade();
            }
        }
        onExited: {
            if(!highlighted)
            {
                animationGradientActive.stop();
                animationGradientInactive.start();
                fadedRectangle.stopFade()
                fadedText.stopFade();
            }
        }

        hoverEnabled: headerText !== ""
        cursorShape: headerText !== "" ? Qt.PointingHandCursor : Qt.ArrowCursor
    }
}
