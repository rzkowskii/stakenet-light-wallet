import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import com.xsn.utils 1.0

ColumnLayout {
    spacing: 10
    property string process: ""
    FontLoader { id: mediumFont; source: "qrc:/Rubik-Medium.ttf" }

    Item {
        Layout.fillHeight: true
        Layout.fillWidth: true
    }

    XSNLabel {
        Layout.leftMargin: isMobile ? 25 : 300
        Layout.preferredHeight: 30
        text: "Loading %1 ..." .arg(process)
        font.family: mediumFont.name
        font.pixelSize: 20
    }

    ProgressBar {
        id: progressBar
        Layout.alignment: Qt.AlignCenter
        Layout.preferredHeight: 10
        Layout.fillWidth: true
        Layout.rightMargin: isMobile ? 25 : 300
        Layout.leftMargin: isMobile ? 25 : 300
        indeterminate: true

        background:  Rectangle {
            anchors.fill: parent
            color: SkinColors.menuBackgroundGradienRightLine
            radius: 2
            opacity: 0.7
        }
    }

    Item {
        Layout.fillHeight: true
        Layout.fillWidth: true
    }
}

