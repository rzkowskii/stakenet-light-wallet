import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12

import "../Components"

import com.xsn.utils 1.0

Rectangle {
    color: "transparent"
    border.color: SkinColors.popupFieldBorder
    border.width: 1

    property alias title: titleLabel.text
    property string helpIconText: ""

    FontLoader {
        id: regularFont
        source: "qrc:/Rubik-Regular.ttf"
    }

    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.topMargin: -titleLabel.implicitHeight / 2
        anchors.leftMargin: 15
        height: titleLabel.contentHeight
        width: titleLabel.contentWidth + helpIcon.width + 10
        color: SkinColors.popupsBgColor

        Row {
            anchors.centerIn: parent
            spacing: 3

            XSNLabel {
                id: titleLabel
                font.family: regularFont.name
                font.pixelSize: 12
                color: SkinColors.secondaryText
            }

            Image {
                id: helpIcon
                source: "qrc:/images/ic_help.png"

                MouseArea {
                    id: helpIconMouseArea
                    hoverEnabled: true
                    anchors.fill: parent
                }
            }

            CustomToolTip {
                id: customToolTip
                width: 570
                parent: helpIcon
                visible: helpIconMouseArea.containsMouse && helpIconText !==""
                tailPosition: 250
                text: helpIconText
            }
        }
    }
}
