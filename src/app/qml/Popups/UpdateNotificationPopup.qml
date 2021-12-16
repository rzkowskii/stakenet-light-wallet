import QtQuick 2.6
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2

import "../Components"
import "../Views"

import com.xsn.viewmodels 1.0
import com.xsn.models 1.0
import com.xsn.utils 1.0

ActionDialog {
    id: root
    width: 650
    height: 480

    property alias confirmButtonText: activateButton.text

    FontLoader {
        id: fontRegular
        source: "qrc:/Rubik-Regular.ttf"
    }

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 35

        Image {
            Layout.alignment: Qt.AlignHCenter
            source: "qrc:/images/updatesoftware.svg"
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            spacing: 20

            Text {
                text: "A new version of Stakenet DEX is available!"
                font.family: fontRegular.name
                font.pixelSize: 24
                color: SkinColors.mainText
            }

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: ""
                font.family: fontRegular.name
                font.pixelSize: 16
                color: SkinColors.mainText
                horizontalAlignment: Text.AlignHCenter
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 45

            IntroButton {
                Layout.preferredHeight: 40
                Layout.preferredWidth: 200
                buttonHoveredColor: SkinColors.cancelButtonHoveredBgrColor
                buttonColor: SkinColors.cancelButtonColor

                buttonGradientHoveredColor: SkinColors.cancelButtonGradientHoveredColor
                buttonGradientColor: SkinColors.cancelButtonGradienColor
                font.pixelSize: 12
                text: "Later"
                onClicked: reject()
            }

            IntroButton {
                id: activateButton
                Layout.preferredHeight: 40
                Layout.preferredWidth: 200
                font.pixelSize: 12
                text: "Update"
                onClicked: accept()
            }
        }
    }
}
