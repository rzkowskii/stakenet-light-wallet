import QtQuick 2.6
import QtQuick.Layouts 1.3

import com.xsn.utils 1.0

ColumnLayout {
    id: layout
    property string operationMsg: ""
    property string resultMsg: ""
    property string imgPath: ""
    property var confirmBtnAction
    property string txID: ""

    FontLoader {
        id: mediumFont
        source: "qrc:/Rubik-Medium.ttf"
    }

    spacing: 30

    Item {
        Layout.alignment: Qt.AlignHCenter
        Layout.preferredHeight: 110
        Layout.fillWidth: true

        ColorOverlayImage {
            anchors.centerIn: parent
            imageSize: 110
            color: SkinColors.mainText
            imageSource: imgPath
        }
    }

    ColumnLayout {
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignHCenter
        spacing: 10

        XSNLabel {
            Layout.alignment: Qt.AlignHCenter
            font.family: mediumFont.name
            font.pixelSize: 22
            text: resultMsg
        }

        ColumnLayout {
            Layout.preferredHeight: 70
            Layout.preferredWidth: 550
            Layout.maximumWidth: layout.width
            Layout.alignment: Qt.AlignHCenter
            spacing: 3

            SelectedText {
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
                font.family: mediumFont.name
                font.pixelSize: 14
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                wrapMode: Text.WordWrap
                text: operationMsg
                color: SkinColors.mainText
            }

            CopiedField {
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
                visible: txID !== ""
                Layout.preferredHeight: 30
                bgColor: SkinColors.mainBackground
                text: txID
            }

        }
    }

    IntroButton {
        id: confirmBtn
        Layout.preferredWidth: 210
        Layout.preferredHeight: 45
        text: qsTr("OK")
        Layout.alignment: Qt.AlignHCenter | Qt.AlignBottom
        onClicked: root.close()
    }
}
