import QtQuick 2.6
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2

import "../Components"

import com.xsn.viewmodels 1.0
import com.xsn.models 1.0
import com.xsn.utils 1.0

Item {
    id: root

    property SyncStateProvider stateProvider

    FontLoader { id: regularFont; source: "qrc:/Rubik-Regular.ttf" }

    Rectangle {
        anchors.fill: parent
        color: SkinColors.popupsBgColor
    }

    RowLayout {
        anchors.fill: parent
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: 24
        anchors.rightMargin: 24
        spacing: 20

        XSNLabel {
            text: "Rescanning..."
            font.family: regularFont.name
            font.pixelSize: 12
            color: SkinColors.mainText
        }

        XSNLabel {
            text: "Fetching latest data from the Blockchain"
            font.family: regularFont.name
            font.pixelSize: 12
            color: SkinColors.secondaryText
        }

        XSNLabel {
            text: stateProvider ? "%1%".arg(stateProvider.bestBlockHeight !== 0 ? parseInt(stateProvider.rescanProgress / stateProvider.bestBlockHeight * 100) : 0) : ""
            font.pixelSize: 12
            font.family: regularFont.name
            font.weight: Font.Light
            color: SkinColors.sendPopupConfirmText
        }

        ProgressBar {
            id: progressBar
            Layout.fillWidth: true
            Layout.preferredHeight: 8
            value: stateProvider ? stateProvider.rescanProgress / stateProvider.bestBlockHeight : 0

            background:  Rectangle {
                anchors.fill: parent
                color: SkinColors.mainBackground
                radius: 2
            }

            contentItem: Item {
                anchors.fill: parent

                Rectangle {
                    width:  progressBar.visualPosition * progressBar.width
                    height: progressBar.height
                    radius: 2
                    color: SkinColors.menuBackgroundGradienRightLine
                }
            }
        }

        XSNLabel {
            Layout.preferredWidth: 150
            Layout.alignment: Qt.AlignRight
            text: stateProvider ? "Block %1 of %2".arg(stateProvider.rescanProgress).arg(stateProvider.bestBlockHeight) : ""
            font.family: regularFont.name
            font.pixelSize: 12
            color: SkinColors.secondaryText
            horizontalAlignment: Text.AlignRight
        }
    }
}
