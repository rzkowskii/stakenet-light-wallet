import QtQuick 2.12
import QtQuick.Layouts 1.3

import com.xsn.utils 1.0

RowLayout {
    Layout.fillWidth: true
    Layout.preferredHeight: 20

    property alias leftButton: left
    property alias rightButton: right

    signal leftButtonClicked()
    signal rightButtonClicked()

    Item {
        Layout.preferredWidth: 70
        Layout.preferredHeight: 20

        PointingCursorMouseArea {
            onClicked: leftButtonClicked()
        }

        RowLayout {
            anchors.fill: parent
            spacing: 10

            Item {
                width: 17
                height: 20
                visible: left.visible

                ColorOverlayImage {
                    anchors.centerIn: parent
                    anchors.fill: parent
                    imageSize: parent.width
                    imageSource: "qrc:/images/backImage.png"
                    color: SkinColors.mainText
                }
            }

            Text {
                id: left
                Layout.alignment: Qt.AlignVCenter & Qt.AlignTop
                font.capitalization: Font.AllUppercase
                font.pixelSize: 16
                font.family: regularFont.name
                color: SkinColors.mainText
            }
        }
    }

    Item {
        Layout.fillWidth: true
    }

    Item {
        Layout.preferredWidth: 70
        Layout.preferredHeight: 20
        Layout.alignment: Qt.AlignRight

        PointingCursorMouseArea {
            onClicked: rightButtonClicked()
        }

        Text {
            id: right
            anchors.right: parent.right
            font.capitalization: Font.AllUppercase
            font.pixelSize: 16
            font.family: regularFont.name
            color: SkinColors.mainText
        }
    }
}
