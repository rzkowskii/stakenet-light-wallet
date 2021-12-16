import QtQuick 2.1
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtQuick.Controls.Styles 1.4

import "../Components"

ActionDialog {
    id: root
    property var model: undefined
    popUpText: "Transactions synchronization"
    width: 500
    height: 250

    ColumnLayout {
        anchors.fill: root.container
        anchors.margins: 30
        spacing: 15

        XSNLabel {
            Layout.fillWidth: true
            text: "Loading....."
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        ProgressBar {
            id: progressBar
            Layout.fillWidth: true
            Layout.preferredHeight: 8
            anchors.verticalCenter: parent.verticalCenter

            background:  Rectangle {
                anchors.fill: parent
                color: "white"
                radius: 2
            }

            contentItem: Item {
               anchors.fill: parent

                Rectangle {
                    width:  progressBar.visualPosition * progressBar.width
                    height: progressBar.height
                    radius: 2
                    color: SkinColors.transactionItemReceived
                }
            }
            value: 82
            to: 100
        }

        RowLayout {
            Layout.fillWidth: true

            XSNLabel {
                text: "%1%" .arg("82")
                font.pixelSize: 16
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            XSNLabel {
                text: "%1 / %2" .arg("82") .arg("100")
                font.pixelSize: 16
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}


