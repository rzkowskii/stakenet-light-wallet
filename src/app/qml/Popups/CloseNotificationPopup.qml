import QtQuick 2.6
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import "../Components"

import com.xsn.viewmodels 1.0
import com.xsn.models 1.0
import com.xsn.utils 1.0

ActionDialog {
    id: root
    width: 480
    height: 250

    property string message
    signal confirmClosing();

    FontLoader { id: regularFont; source: "qrc:/Rubik-Regular.ttf" }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 30
        spacing: 25

        Text {
            Layout.fillWidth: true
            Layout.preferredHeight: 35
            color: SkinColors.mainText
            lineHeight: 1.5
            wrapMode: Text.WordWrap
            font.family: regularFont.name
            font.pixelSize: 18
            horizontalAlignment: Text.AlignHCenter
            text: message
        }

        RowLayout {
            Layout.preferredHeight: 20
            Layout.fillWidth: true
            Layout.leftMargin: 20
            spacing: 5
            visible: false

            CheckBox {
                id: checkbox
                checked: false

                indicator: Rectangle {
                    implicitWidth: 17
                    implicitHeight: 17
                    x: checkbox.leftPadding
                    y: parent.height / 2 - height / 2
                    radius: 6
                    border.color: SkinColors.mainText
                    color: "transparent"
                    border.width: 1


                    ColorOverlayImage {
                        width: 15
                        height: 15
                        x: 2
                        y: 1
                        imageSize: 15
                        color: SkinColors.mainText
                        imageSource: "qrc:/images/checkmark-round.png"
                        visible: checkbox.checked
                    }
                }

                PointingCursorMouseArea {
                    onClicked:  {
                        checkbox.checked = !checkbox.checked
                    }

                    onEntered:  {
                        checkbox.scale = scale * 1.1
                    }
                    onExited: {
                        checkbox.scale = scale
                    }
                }
            }

            Text {
                font.weight: Font.Thin
                text: "Ignore this box in the future"
                font.family: regularFont.name
                font.pixelSize: 12
                color: SkinColors.mainText

            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            spacing: 25

            IntroButton {
                Layout.preferredHeight: 40
                Layout.preferredWidth: 150
                font.pixelSize: 12
                text: "Cancel"
                onClicked: close()
            }

            IntroButton {
                id: confirmButton
                Layout.preferredHeight: 40
                Layout.preferredWidth: 150
                font.pixelSize: 12
                text: "Ok"
                onClicked: confirmClosing();
            }
        }
    }
}
