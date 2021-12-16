import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12

import "../Components"
import "../Views"

import com.xsn.models 1.0
import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

Rectangle {
    id: root
    property var model: undefined
    property alias currentIndex: listView.currentIndex

    color: SkinColors.mainBackground
    radius: 15
    border.width: 1
    border.color: SkinColors.popupFieldBorder

    ListView {
        id: listView
        currentIndex: root.currentIndex
        anchors.fill: parent
        clip: true
        model: root.model
        boundsBehavior: Flickable.StopAtBounds
        orientation: ListView.Horizontal
        highlightFollowsCurrentItem: true
        spacing: 0
        
        highlight : Item {
            anchors.fill: parent
            
            Rectangle {
                anchors.leftMargin: 2
                anchors.rightMargin: 2
                width: listView.currentItem ? listView.currentItem.width : 0
                height: listView.currentItem ? listView.currentItem.height : 0
                x: listView.currentItem ? listView.currentItem.x : 0
                y: listView.currentItem ? listView.currentItem.y : 0
                visible: listView.currentItem
                anchors.verticalCenter: listView.currentItem ? parent.verticalCenter :
                                                               undefined
                
                color: SkinColors.menuBackgroundGradienRightLine
                radius: 15
            }
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
                font.pixelSize: 12
                color: delegateItem.ListView.isCurrentItem ? SkinColors.mainText : SkinColors.secondaryText
            }
            
            PointingCursorMouseArea {
                id: mouseArea
                onClicked: {
                    root.currentIndex = index;
                }
            }
        }
    }
}
