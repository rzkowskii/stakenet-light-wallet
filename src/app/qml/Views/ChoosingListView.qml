import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import "../Components"
import com.xsn.utils 1.0

Rectangle {
    id: root
    property alias model: listView.model
    property string currentOption: "Medium"
    property alias currentIndex: listView.currentIndex
    property string highlighRectangleColor: SkinColors.menuBackgroundGradientFirst
    property string highlightBorderColor: SkinColors.mobileButtonBackground
    property int textItemPixelSize: 14
    property var textItemCapitalization: Font.MixedCase

    color: SkinColors.secondaryBackground
    radius: 20

    ListView {
        id: listView
        anchors.fill: parent
        orientation: ListView.Horizontal
        boundsBehavior: Flickable.StopAtBounds
        spacing: 0
        onCurrentIndexChanged: {
            if(currentIndex === -1) {
                currentOption = ""
            }
        }

        highlightFollowsCurrentItem: true
        highlight : Rectangle {
            anchors.leftMargin: 3
            anchors.rightMargin: 3
            anchors.verticalCenter: parent !== null ? parent.verticalCenter : undefined
            color: highlightBorderColor
            radius: 18
            border.width: 1
            border.color: highlightBorderColor
        }


        delegate: Item {
            id: delegateItem
            anchors.verticalCenter: parent.verticalCenter
            width: root.width / root.model.length
            height: parent.height - 4

            SecondaryLabel {
                anchors.centerIn: parent
                text: modelData
                font.family: regularFont.name
                font.pixelSize: textItemPixelSize
                font.capitalization: textItemCapitalization
                color: delegateItem.ListView.isCurrentItem ? "white" : SkinColors.menuItemText
            }

            PointingCursorMouseArea {
                id: mouseArea
                onClicked: {
                    listView.currentIndex = index;
                    currentOption = modelData;
                }
            }
        }
    }
}

