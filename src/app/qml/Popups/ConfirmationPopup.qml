import QtQuick 2.6
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import "../Components"

import com.xsn.utils 1.0

ActionDialog {
    id: root

    property alias messageItem: messageField
    property alias cancelButton: cancelBtn
    property alias confirmButton: confirmBtn

    signal confirmClicked();
    signal cancelClicked();

    property string message: ""

    FontLoader { id: regularFont; source: "qrc:/Rubik-Regular.ttf" }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 30
        spacing: 25

        Text {
            id: messageField
            Layout.fillWidth: true
            Layout.preferredHeight: 35
            color: SkinColors.mainText
            wrapMode: Text.WordWrap
            font.family: regularFont.name
            font.pixelSize: 18
            horizontalAlignment: Text.AlignHCenter
            text: message
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            spacing: 25

            IntroButton {
                id: cancelBtn
                Layout.preferredHeight: 40
                Layout.preferredWidth: 150
                font.pixelSize: 12
                onClicked: cancelClicked();
            }

            IntroButton {
                id: confirmBtn
                Layout.preferredHeight: 40
                Layout.preferredWidth: 150
                font.pixelSize: 12
                onClicked: confirmClicked();
            }
        }
    }
}
