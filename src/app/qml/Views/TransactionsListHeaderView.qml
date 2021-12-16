import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0

import com.xsn.utils 1.0

import "../Components"

RoundedRectangle {
    id: root
    property string currentSortRole: layout.children[currentIndex].sortRole ? layout.children[currentIndex].sortRole : ""
    property bool orderAsc: false
    property int currentIndex: 1
    property ListModel tabsModel
    corners.topLeftRadius: 10
    corners.topRightRadius: 10
    customGradient: {
        "vertical": true,
        "colors": [{
                       "position": 0.0,
                       "color": SkinColors.delegatesBackgroundLightColor
                   }, {
                       "position": 1.0,
                       "color": SkinColors.delegatesBackgroundDarkColor
                   }]
    }

    RowLayout {
        id: layout
        anchors.fill: parent
        anchors.leftMargin: 25
        anchors.rightMargin: 15
        spacing: 0

        Repeater {
            id: tabsRepeater
            model: tabsModel

            delegate: Item {
                id: delegateItem
                Layout.preferredWidth: parent.width * model.size
                Layout.fillHeight: true

                property string sortRole: model.role
                property bool isCurrentItem: currentIndex == index

                RowLayout {
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 8

                    FadedText {
                        id: fadedText
                        Layout.alignment: Qt.AlignVCenter
                        isCurrentItem: delegateItem.isCurrentItem
                        color: isCurrentItem ? SkinColors.mainText : SkinColors.headerText
                        font.pixelSize: 11
                        text: model.name
                    }

                    ColumnLayout {
                        spacing: 4

                        CustomArrow {
                            id: upArrow
                            Layout.alignment: Qt.AlignHCenter
                            Layout.preferredHeight: 5
                            Layout.preferredWidth: 5
                            color: delegateItem.isCurrentItem ? (orderAsc ? SkinColors.mainText : SkinColors.headerText) : SkinColors.headerText
                            rotation: -90
                        }

                        CustomArrow {
                            id: downArrow
                            Layout.alignment: Qt.AlignHCenter
                            Layout.preferredHeight: 5
                            Layout.preferredWidth: 5
                            color: delegateItem.isCurrentItem ? (orderAsc ? SkinColors.headerText : SkinColors.mainText) : SkinColors.headerText
                            rotation: 90
                        }
                    }
                }

                PointingCursorMouseArea {
                    id: mouseArea
                    hoverEnabled: fadedText.text !== ""
                    cursorShape: fadedText.text !== "" ? Qt.PointingHandCursor : Qt.ArrowCursor
                    onClicked: {
                        if (delegateItem.isCurrentItem) {
                            orderAsc = !orderAsc
                        } else {
                            currentIndex = index
                            orderAsc = true
                        }
                    }
                    onEntered: {
                        if (!delegateItem.isCurrentItem) {
                            fadedText.startFade()
                        }
                    }
                    onExited: {
                        if (!delegateItem.isCurrentItem) {
                            fadedText.stopFade()
                        }
                    }
                }
            }
        }

        Item {
            Layout.fillWidth: true
        }
    }
}
