import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import "../Components"

import com.xsn.utils 1.0

Button {
    id: control
    property alias title: title
    property alias backgroundButton: backgr
    property alias image: image

    checkable: true
    font.pixelSize: 12
    FontLoader { id: regularFont; source: "qrc:/Rubik-Regular.ttf" }

    contentItem: Item {
        anchors.fill: parent

        Row {
            anchors.centerIn: parent
            spacing: 0

            Item {
                width: image.imageSize
                height: image.imageSize
                visible: image.imageSource

                ColorOverlayImage {
                    id: image
                    width: imageSize
                    height: imageSize
                    anchors.centerIn: parent
                }
            }

            Text {
                id: title
                visible: title.text !== ""
                font.family: regularFont.name
                opacity: enabled ? 1.0 : 0.3
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                color: SkinColors.mainText
                text: title.text
                font.pixelSize: 12
            }
        }
    }

    background: Rectangle {
        id: backgr
        implicitWidth: control.width
        implicitHeight: control.height
        border.width: 1
    }
}
