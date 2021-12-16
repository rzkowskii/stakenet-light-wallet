import QtQuick 2.4
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2

import Components 1.0

import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

Item {
    property alias passLenImgSource: imgLen.source
    property alias passNumberImgSource: imgNumber.source
    property alias passLetterImgSource: imgLetter.source

    ColumnLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        spacing: 5

        XSNLabel {
            Layout.preferredHeight: 20
            Layout.fillWidth: true
            font.pixelSize: 16
            text: "Your password needs to:"
        }

        Row {
            spacing: 5

            Image {
                id: imgLen
                sourceSize: Qt.size(20, 20)
            }

            XSNLabel {
                Layout.fillWidth: true
                font.pixelSize: 12
                text: "be at least 8 characters long."
            }
        }

        Row {
            spacing: 5

            Image {
                id: imgNumber
                sourceSize: Qt.size(20, 20)
            }

            XSNLabel {
                Layout.preferredHeight: 20
                Layout.fillWidth: true
                font.pixelSize: 12
                text: "include at least one number or symbol."
            }
        }

        Row {
            spacing: 5

            Image {
                id: imgLetter
                sourceSize: Qt.size(20, 20)
            }

            XSNLabel {
                Layout.preferredHeight: 20
                Layout.fillWidth: true
                font.pixelSize: 12
                text: "include both lower and upper case characters."
            }
        }
    }

}
