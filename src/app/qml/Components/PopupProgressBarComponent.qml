import QtQuick 2.6
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.0

import com.xsn.utils 1.0

ColumnLayout {
    id: layout

    property string progressBarMsg: ""

    FontLoader { id: mediumFont; source: "qrc:/Rubik-Medium.ttf" }

    spacing: 30

    Item {
        Layout.fillHeight: true
        Layout.fillWidth: true
    }

    XSNLabel {
        text: progressBarMsg
        font.family: mediumFont.name
        font.pixelSize: 20
        visible: progressBarMsg !== ""
    }

    ProgressBar {
        id: progressBar
        indeterminate: true
        Layout.preferredHeight: 10
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignCenter

        background:  Rectangle {
            anchors.fill: parent
            color: SkinColors.sendPopupConfirmText
            radius: 2
            opacity: 0.7
        }
    }

    Item {
        Layout.fillHeight: true
        Layout.fillWidth: true
    }
}
