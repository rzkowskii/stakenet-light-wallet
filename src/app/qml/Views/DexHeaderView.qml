import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0

import "../Components"

import com.xsn.utils 1.0

Item {
    id: root
    property alias model: listView.model
    property alias currentIndex: listView.currentIndex

    Rectangle {
        id: back
        anchors.fill: parent
        color: SkinColors.dexPageSecondaryBackground
        radius: 7
    }

    ListView {
        id: listView
        anchors.fill: parent
        clip: true
        orientation: ListView.Horizontal
        boundsBehavior: Flickable.StopAtBounds
        spacing: 0
        currentIndex: 0

        highlightFollowsCurrentItem: true

        highlight: Item {
            anchors.fill: parent

            Rectangle {
                id: bgRectangle
                radius: 7
                width: listView.currentItem ? listView.currentItem.width : 0
                height: listView.currentItem ? listView.currentItem.height : 0
                x: listView.currentItem ? listView.currentItem.x : 0
                y: listView.currentItem ? listView.currentItem.y : 0
                visible: listView.currentItem
                anchors.verticalCenter: listView.currentItem ? parent.verticalCenter : undefined
                color: "transparent"

                LinearGradient {
                    source: Rectangle {
                        width: bgRectangle.width
                        height: bgRectangle.height
                        color: "white"
                        radius: 7
                    }
                    anchors.fill: parent
                    start: Qt.point(0, 0)
                    end: Qt.point(0, height)
                    opacity: 0.12
                    gradient: Gradient {
                        GradientStop {
                            position: 1.0
                            color: "transparent"
                        }
                        GradientStop {
                            id: grad
                            position: 0.0
                            color: "white"
                        }
                    }
                }
            }
        }

        delegate: Item {
            id: delegate
            width: model.size
            height: 30

            Text {
                id: fadedText
                anchors.centerIn: parent
                text: model.name
                color: SkinColors.mainText
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                font.family: fontRegular.name
                font.capitalization: Font.MixedCase
                font.pixelSize: 14
            }

            PointingCursorMouseArea {
                id: mouseArea
                onClicked: {
                    listView.currentIndex = index
                }
            }
        }
    }
}
