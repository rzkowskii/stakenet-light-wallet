import QtQuick 2.4
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2

import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

ColumnLayout {
    spacing: 10
    FontLoader { id: mediumFont; source: "qrc:/Rubik-Medium.ttf" }

    Item {
        Layout.fillHeight: true
        Layout.fillWidth: true
    }

  Image {
        Layout.alignment: Qt.AlignHCenter
        source: "qrc:/images/StakenetX.png"
        sourceSize: Qt.size(360, 360)
    }

    XSNLabel {
        Layout.fillWidth: true
        Layout.preferredHeight: 50
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text: "STAKENET DEX"
        font.family: mediumFont.name
        color: SkinColors.mainText
        font.weight: Font.Medium
        font.pixelSize: 40
    }

    Item {
        Layout.fillHeight: true
        Layout.fillWidth: true
    }
}
