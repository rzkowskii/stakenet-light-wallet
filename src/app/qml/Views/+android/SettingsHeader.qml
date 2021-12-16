import QtQuick 2.12
import QtQuick.Layouts 1.3

import com.xsn.utils 1.0
import com.xsn.viewmodels 1.0

ColumnLayout {
    Layout.alignment: Qt.AlignHCenter
    spacing: 10

    Image {
        Layout.alignment: Qt.AlignHCenter
        source: "qrc:/images/ic_logox.png"
        sourceSize: Qt.size(64, 73)
    }

    Text {
        Layout.alignment: Qt.AlignHCenter
        text: "Stakenet DEX"
        font.pixelSize: 16
        font.family: mediumFont.name
        color: SkinColors.mainText
        font.bold: true

    }

    Text {
        Layout.alignment: Qt.AlignHCenter
        text: "V %1".arg(appVersion)
        font.pixelSize: 12
        font.family: fontRegular.name
        color: SkinColors.menuItemText
    }
}
