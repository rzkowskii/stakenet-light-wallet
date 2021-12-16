import QtQuick 2.6
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2

import com.xsn.utils 1.0

Rectangle {
    anchors.fill: parent
    color: SkinColors.mainBackground
    border.width: 1

    property string coinName: ""
    property string coinAmount: ""
    property string coinValue: ""

    RowLayout {
        anchors.fill: parent

        Image {
            Layout.leftMargin: 10
            source: coinName !== "" ? "qrc:/images/ICON_%1.svg" .arg(coinName) : ""
            sourceSize: Qt.size(20, 25)
        }

        Text {
            id: name
            text: coinName
            font.bold: true
            color: SkinColors.mainText
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        SecondaryLabel {
            font.pixelSize: 12
            text: coinAmount + qsTr(" ") + coinValue
        }

        RoundedImage {
            Layout.rightMargin: 10
            imageSource: "qrc:/images/check@2x.png"
            sourceSize: Qt.size(10, 10)
        }
    }
}
