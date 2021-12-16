import QtQuick 2.12
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.0
import QtQuick.Layouts 1.3


import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

Button {
    id: actionButton

    property string buttonText: ""
    property string source: ""
    property var buttonColor: undefined
    property int buttonRadius: 20
    property int buttonSpacing: 6
    property int iconWidth: 16
    property int iconHeight: 16
    property alias context: itemText

    padding: 15

    background: Rectangle {
        radius: buttonRadius
        border.width: 1
        border.color: buttonColor
        color: "transparent"

        Rectangle {
            width: parent.width - border.width
            height: parent.height - border.width
            color: buttonColor
            opacity: 0.2
            radius: buttonRadius
        }
    }

    contentItem: Item {
        anchors.fill: parent

        Row {
            spacing: buttonSpacing
            anchors.centerIn: parent

            Item {
                visible: source !== ""
                id: icon
                width: iconWidth
                height: iconHeight

                Image {
                    id: image
                    anchors.fill: parent
                    sourceSize: Qt.size(iconWidth, iconHeight)
                    source: actionButton.source
                }

                ColorOverlay {
                    anchors.fill: image
                    source: image
                    color: "#FFFFFF"
                }
            }

            XSNLabel {
                id: itemText
                anchors.verticalCenter: parent.verticalCenter
                FontLoader { id: localFont; source: "qrc:/Rubik-Regular.ttf" }
                font.family: localFont.name
                text: actionButton.buttonText
                color: SkinColors.mainText
                font.pixelSize: 14
                font.capitalization: Font.AllUppercase
            }
        }
    }
}

