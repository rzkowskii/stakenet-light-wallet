import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import "../Components"
import com.xsn.utils 1.0

Item {
    id: root
    property var model: undefined
    property string currentOption: ""
    property alias currentIndex: listView.currentIndex
    property alias color: back.color
    property alias radius: back.radius
    property alias opacityBackground: back.opacity
    signal clicked(string percent)

    Rectangle {
        id: back
        anchors.fill: parent
        color: SkinColors.dexPageSecondaryBackground
        radius: 9
    }

    FontLoader { id: regularFont; source: "qrc:/Rubik-Regular.ttf" }

    ListView {
        id: listView
        currentIndex: root.currentIndex
        anchors.fill: parent
        clip: true

        spacing: 0
        orientation: ListView.Horizontal

        highlightFollowsCurrentItem: true
        highlight : Item {
            anchors.fill: parent

            Rectangle {
                width: listView.currentItem ? listView.currentItem.width : 0
                height: listView.currentItem ? listView.currentItem.height : 0
                x: listView.currentItem ? listView.currentItem.x : 0
                y: listView.currentItem ? listView.currentItem.y : 0
                visible: listView.currentItem
                anchors.verticalCenter: listView.currentItem ? parent.verticalCenter :
                                                               undefined

                color: SkinColors.mobileButtonBackground
                radius: 9
            }
        }


        boundsBehavior: Flickable.StopAtBounds

        model: root.model
        delegate: Item {
            id: delegateItem
            anchors.verticalCenter: parent.verticalCenter
            width: root.width / root.model.length
            height: parent.height

            RowLayout {
                anchors.fill: parent

                SecondaryLabel {
                    Layout.leftMargin: 7
                    Layout.alignment: Qt.AlignCenter
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    text: modelData
                    font.family: regularFont.name
                    font.pixelSize: 13
                    color: SkinColors.mainText
                }

                Rectangle {
                   Layout.alignment: Qt.AlignRight
                   width: 1
                   height: 16
                   color: SkinColors.dexPercentViewSeparator
                   opacity: 0.3
                   visible: index !== 4
                }
            }

            PointingCursorMouseArea {
                id: mouseArea
                onClicked: {
                    root.clicked(modelData)
                    root.currentIndex = index;
                }
            }
        }
    }
}

