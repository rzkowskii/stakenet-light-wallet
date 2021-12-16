import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import "../Views"
import "../Components"

import com.xsn.viewmodels 1.0
import com.xsn.models 1.0

import com.xsn.utils 1.0

Component {
    id: refreshDialogComp

    ActionDialog {
        id: root
        width: 420
        height: 230

        property int assetID
        property string coinName: ""
        property double averageSyncTime

        FontLoader { id: regularFont; source: "qrc:/Rubik-Regular.ttf" }

        ColumnLayout {
            anchors.centerIn: parent
            anchors.fill: parent
            anchors.margins: 10
            spacing: 15

            CloseButton {
                Layout.preferredHeight: 20
                Layout.preferredWidth: 20
                Layout.alignment: Qt.AlignRight | Qt.AlignTop
                onClicked: close()
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignHCenter
                spacing: 10

                Label {
                    Layout.fillWidth: true
                    lineHeight: 1.5
                    wrapMode: Text.WordWrap
                    color: SkinColors.mainText
                    font.family: regularFont.name
                    font.pixelSize: 16
                    horizontalAlignment: Text.AlignHCenter
                    text: "Are you sure you want to rescan \n the %1 blockchain?".arg(coinName)
                }

                Label {
                    property int avarageMin: averageSyncTime > 60 ? averageSyncTime / 60 : 0
                    Layout.fillWidth: true
                    color: SkinColors.mainText
                    font.family: regularFont.name
                    font.pixelSize: 16
                    horizontalAlignment: Text.AlignHCenter
                    text: 'It will take about <font color="%1"> %2 %3 %4 s </font>'.arg(
                              SkinColors.sendPopupConfirmText).arg(
                              avarageMin > 0 ? avarageMin : "").arg(
                              avarageMin > 0 ? "min" : "").arg(
                              avarageMin > 0 ? Math.ceil(averageSyncTime - avarageMin * 60) : Math.ceil(averageSyncTime))
                }

                Item {
                    Layout.fillHeight: true
                }
            }

            IntroButton {
                Layout.preferredHeight: 40
                Layout.preferredWidth: 160
                Layout.alignment: Qt.AlignHCenter
                font.pixelSize: 12
                text: "Ok"
                onClicked: {
                    ApplicationViewModel.syncService.rescanChain(assetID)
                    close();
                }
            }
        }
    }
}
