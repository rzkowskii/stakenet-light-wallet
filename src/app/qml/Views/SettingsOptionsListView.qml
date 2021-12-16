import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0

import "../Components"

import com.xsn.utils 1.0

ListView {
    id: settingsListView
    model: ["Assets", "Orderbook UX", "Localization", "Backup", "Software Version"]

    property int actualIndex: 0
    clip: true
    spacing: 0
    boundsBehavior: Flickable.StopAtBounds


    highlight: Item {

        Rectangle {
            anchors.right: parent.right
            height: 40
            width: 2
            color: settingsListView.currentItem ? SkinColors.menuBackgroundGradientFirst : ""
        }
    }

    delegate: Item {
        id: settingsItem
        height: 40
        width: parent.width

        RowLayout {
            anchors.fill: parent

            Text {
                id: assetsName
                text: modelData
                font.pixelSize: 14
                font.family: fontRegular.name
                color: "#FFFFFF"
            }

            Item {
                Layout.fillHeight: true
                Layout.fillWidth: true
            }

            FadedRectangle {
                id: fadeBackground
                Layout.fillWidth: true
                height: 40
                Layout.maximumWidth: 2
                activeStateColor: SkinColors.highlightedMenuItem
                inactiveStateColor: "transparent"
                color: settingsItem.ListView.isCurrentItem ? SkinColors.menuBackgroundGradientFirst : "transparent"
            }
        }

        LinearGradient {
            anchors.fill: parent
            opacity: 0.3
            start: Qt.point(0, 0)
            end: Qt.point(width, 0)
            gradient: Gradient {
                GradientStop {
                    id: grad
                    position: 1.0
                    color: settingsItem.ListView.isCurrentItem ? SkinColors.menuBackgroundGradientFirst : "transparent"

                    AnimationColor on color {
                        id: animationGradientColor
                        propAnimation.to: SkinColors.highlightedMenuItem;
                    }

                    AnimationColor on color {
                        id: animationGradientTransparent
                        propAnimation.to: "transparent"
                    }
                }
                GradientStop { position: 0.0; color: "transparent" }
            }
        }

        PointingCursorMouseArea {
            id: mouseArea
            onClicked: {
                settingsListView.actualIndex = index;
                settingsListView.currentIndex = index;
            }
            onEntered:{
                if(!settingsItem.ListView.isCurrentItem)
                {

                    animationGradientTransparent.stop();
                    animationGradientColor.start();
                    fadeBackground.startFade();
                }
            }

            onExited: {
                if(!settingsItem.ListView.isCurrentItem)
                {
                    animationGradientColor.stop();
                    animationGradientTransparent.start();
                    fadeBackground.stopFade();
                }
            }
        }
    }
}
