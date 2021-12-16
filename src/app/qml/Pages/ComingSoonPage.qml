import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import "../Components"

import com.xsn.utils 1.0

Page {
    id: root
    property alias rootItem: item

    background: Image {
        source: "qrc:/images/ComingSoonPageBg.svg"
    }

    FontLoader {
        id: fontMedium
        source: "qrc:/Rubik-Medium.ttf"
    }

    Item {
        id: item
        anchors.fill: parent

        ColumnLayout {
            anchors.topMargin: -20
            anchors.centerIn: parent
            spacing: 15

            Image {
                Layout.alignment: Qt.AlignHCenter
                sourceSize: Qt.size(item.width * 0.13, item.width * 0.13 * 1.15)
                source: "qrc:/images/ComingSoonIcon.svg"
            }

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: "Coming soon"
                font.capitalization: Font.Capitalize
                font.family: fontMedium.name
                font.pixelSize: 23
                color: SkinColors.mainText
            }
        }
    }
}
