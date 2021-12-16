import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2

import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

FadedRectangle {
    id: root
    property string text: ""
    property string bgColor: ""
    property alias textColor: from.color
    activeStateColor: bgColor
    inactiveStateColor: "transparent"
    radius: 4

    MouseArea {
        id: mouseArea
        height: root.height
        width: root.width - copyImg.width
        hoverEnabled: true
        onEntered: {
            animationBgActive.start();
            root.startFade()
        }
        onExited: {
            animationBgInactive.start();
            root.stopFade()
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 5

        XSNLabel {
            id: from
            Layout.fillWidth: true
            Layout.leftMargin: 10
            width: parent.width
            text: root.text
            font.pixelSize: 13
            elide: Text.ElideRight
        }

        ColorOverlayImage {
            id: copyImg
            Layout.preferredWidth: 20
            Layout.preferredHeight: 20
            Layout.margins: 5
            visible: mouseArea.containsMouse || pntMs.containsMouse
            imageSource: "qrc:/images/copy.png"
            imageSize: 20

            AnimationColor on color {
                id: animationBgActive
                propAnimation.to: SkinColors.headerText;
            }

            AnimationColor on color {
                id: animationBgInactive
                propAnimation.to: "transparent"
            }

            PointingCursorMouseArea {
                id: pntMs
                hoverEnabled: true
                onClicked: {
                    Clipboard.setText(from.text)
                    showBubblePopup("Copied");
                }
                onEntered: {
                    animationBgActive.start();
                    root.startFade()
                }
                onExited: {
                    animationBgInactive.start();
                    root.stopFade()
                }
            }
        }
    }
}
