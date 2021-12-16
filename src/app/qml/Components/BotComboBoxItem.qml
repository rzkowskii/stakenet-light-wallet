import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12

import "../Components"

import com.xsn.utils 1.0

Item {
    property string coinName: ""
    property string coinSymbol: ""

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 25
        spacing: 10

        Image {
            source: coinName !== "" ? "qrc:/images/ICON_%1.svg" .arg(coinName) : ""
            sourceSize: Qt.size(22, 24)
        }

        Text {
            id: name
            Layout.fillWidth: true
            text: coinSymbol
            color: SkinColors.mainText
            font.family: fontRegular.name
            font.pixelSize: 15
        }
    }
}
