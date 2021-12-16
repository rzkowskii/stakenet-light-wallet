import QtQuick 2.1
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.0

import com.xsn.utils 1.0

import "../Components"

ActionDialog {
    id: root
    property string headerName: ""
    property string explanation: ""
    popUpText: headerName
    width: 500
    height: 180

    FontLoader { id: mediumFont; source: "qrc:/Rubik-Medium.ttf" }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            XSNLabel {
                Layout.fillWidth: true
                text: headerName
                font.family: mediumFont.name
                font.pixelSize: 20
                color: SkinColors.mainText
            }

            CloseButton {
                Layout.preferredHeight: 30
                Layout.preferredWidth: 30
                Layout.alignment: Qt.AlignRight | Qt.AlignTop
                onClicked: root.close()
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            XSNLabel {
                anchors.fill: parent
                anchors.verticalCenter: parent.verticalCenter
                font.pixelSize: 14
                wrapMode: Text.WordWrap
                text: explanation
            }
        }
    }
}

