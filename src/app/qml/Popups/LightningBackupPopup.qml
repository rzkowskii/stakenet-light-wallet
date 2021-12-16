import QtQuick 2.6
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.5
import QtQuick.Dialogs 1.0
import "../Components"

import com.xsn.viewmodels 1.0
import com.xsn.models 1.0
import com.xsn.utils 1.0

ActionDialog {
    id: root
    popUpText: "LN Backup"
    width: 700
    height: 450

    property int assetID

    property string currentName: ""
    property string currentSymbol: ""

    clip: true

    LightningChannelBackupViewModel {
        id: backupViewModel

        Component.onCompleted: {
            backupViewModel.initialize(ApplicationViewModel, assetID);
        }

        onLnChannelsBackupRestored: {
            stackView.push("qrc:/Components/OperationResultComponent.qml", { operationMsg: "Success! Data was successfully restored. Please wait until all channels are closed for your funds to appear in your wallet!",
                               resultMsg : "Success!", imgPath: "qrc:/images/SUCCESS_ICON.png", confirmBtnAction:  function(){ root.close()}});
        }

        onLnChannelsBackupRestoreFailed: {
            stackView.push("qrc:/Components/OperationResultComponent.qml", { operationMsg: "Couldn't restore backup data!",
                               resultMsg : "Failed!" , imgPath: "qrc:/images/crossIcon.png", confirmBtnAction:  function(){ root.close()}});
        }

        onLnChannelsBackupExported: {
            stackView.push("qrc:/Components/OperationResultComponent.qml", { operationMsg: "Backup file was exported!",
                               resultMsg : "Success!", imgPath: "qrc:/images/SUCCESS_ICON.png", confirmBtnAction:  function(){ root.close()}});
        }

        onLnChannelsBackupExportFailed: {
            stackView.push("qrc:/Components/OperationResultComponent.qml", { operationMsg: "Failed to export backup file!",
                               resultMsg : "Failed!" , imgPath: "qrc:/images/crossIcon.png", confirmBtnAction:  function(){ root.close()}});
        }

    }

    LightningChannelBackupViewModel {
        id: backupViewModelFromFile

        onLnChannelsBackupRestored: {
            stackView.push("qrc:/Components/OperationResultComponent.qml", { operationMsg: "Success! Data was successfully restored. Please wait until all channels are closed for your funds to appear in your wallet",
                               resultMsg : "Success!", imgPath: "qrc:/images/SUCCESS_ICON.png", confirmBtnAction:  function(){ root.close()}});
        }

        onLnChannelsBackupRestoreFailed: {
            stackView.push("qrc:/Components/OperationResultComponent.qml", { operationMsg: "Couldn't restore backup data!",
                               resultMsg : "Failed!" , imgPath: "qrc:/images/crossIcon.png", confirmBtnAction:  function(){ root.close()}});
        }
    }

    StackView {
        id: stackView
        anchors.fill: parent
        anchors.margins: 10

        initialItem: Component {
            id: initialComponent

            ColumnLayout {
                property var backupModel: backupViewModel

                Layout.fillHeight: true
                Layout.fillWidth: true
                spacing: 10

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 16

                    Item {
                        Layout.preferredHeight: 60
                        Layout.preferredWidth: 55

                        Image {
                            anchors.fill: parent
                            source: currentName !== "" ?"qrc:/images/ICON_%1.svg" .arg(currentName) : ""
                        }
                    }

                    XSNLabel {
                        font.family: mediumFont.name
                        font.pixelSize: 20
                        text: "LN Backup %1 - %2" .arg(currentName) .arg(currentSymbol)
                        color: SkinColors.mainText
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    CloseButton {
                        Layout.preferredHeight: 25
                        Layout.preferredWidth: 25
                        Layout.alignment: Qt.AlignRight | Qt.AlignTop
                        onClicked: root.close()
                    }
                }

                Rectangle {
                    Layout.preferredHeight: 1
                    Layout.fillWidth: true
                    color: SkinColors.secondaryText
                }

                Column {
                    Layout.fillWidth: true
                    spacing: 5

                    SecondaryLabel {
                        text: qsTr("Date of backup:")
                        visible: channelsOutpoints.count > 0
                        font.pixelSize: 14
                    }

                    XSNLabel {
                        text: backupModel.backupMeta.timestamp || ""
                        visible: channelsOutpoints.count > 0
                        font.pixelSize: 16
                    }
                }

                RowLayout{
                    Layout.fillWidth: true
                    spacing: 20

                    Column {
                        spacing: 5
                        Layout.preferredHeight: 50

                        SecondaryLabel {
                            text: qsTr("Backup directory:")
                            font.pixelSize: 14
                        }

                        XSNLabel {
                            text: backupModel.backupPath
                            font.pixelSize: 16
                            width: 500
                            elide: Text.ElideMiddle
                        }
                    }

                    Item {
                        Layout.preferredHeight: 40
                        Layout.fillWidth: true
                    }

                    IntroButton {
                        Layout.preferredWidth: 110
                        Layout.preferredHeight: 40
                        font.family: mediumFont.name
                        text: qsTr("Location")
                        onClicked: {
                            changeBaclkupFolder.open();
                        }
                    }

                    FileDialog {
                        id: changeBaclkupFolder
                        title: "Please choose folder"
                        folder: shortcuts.desktop
                        selectFolder: true
                        selectMultiple: false

                        onAccepted: {
                            backupModel.changeBackupDir(changeBaclkupFolder.fileUrl);
                        }
                    }

                }

                SecondaryLabel {
                    Layout.fillWidth: true
                    Layout.preferredHeight: channelsOutpoints.count > 0 ? 20 : 80
                    horizontalAlignment: channelsOutpoints.count > 0 ? Text.AlignLeft : Text.AlignHCenter
                    text: channelsOutpoints.count > 0  ?  "Outpoints: " : "There are no channel data in your latest backup"
                    color: channelsOutpoints.count > 0 ? SkinColors.menuItemText : "red"
                }

                ListView {
                    id: channelsOutpoints
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    visible: count > 0
                    spacing: 0
                    boundsBehavior: Flickable.StopAtBounds
                    clip: true

                    model: backupModel.backupMeta.outpoints || null

                    delegate: Rectangle {
                        id: delegate
                        width: channelsOutpoints.width
                        height: 40
                        color: "transparent"

                        CopiedField {
                            anchors.fill: parent
                            text: modelData
                            bgColor: SkinColors.mainBackground
                        }
                    }
                }

                RowLayout {
                    Layout.preferredHeight: 50
                    Layout.alignment: Qt.AlignRight
                    spacing: 32


                    IntroButton {
                        text: qsTr("Import")
                        Layout.preferredWidth: 150
                        Layout.preferredHeight: 50
                        Layout.alignment: Qt.AlignLeft
                        onClicked: {
                            fileDialog.open();
                        }
                    }

                    IntroButton {
                        text: qsTr("Export")
                        visible: channelsOutpoints.count > 0
                        Layout.preferredWidth: 150
                        Layout.preferredHeight: 50
                        Layout.alignment: Qt.AlignLeft
                        onClicked: {
                            exportFileDial.open();
                        }
                    }


                    Item {
                        Layout.fillWidth: true
                    }

                    IntroButton {
                        Layout.preferredWidth: 110
                        Layout.preferredHeight: 50
                        font.family: mediumFont.name
                        enabled: channelsOutpoints.count > 0
                        text: qsTr("Restore")
                        onClicked: {
                            stackView.push(confirmComponent, {restoreBackupModel: backupModel});
                        }
                    }

                    FileDialog {
                        id: fileDialog
                        title: "Please choose backup file"
                        folder: shortcuts.desktop
                        onAccepted: {
                            backupViewModelFromFile.initializeFromFile(ApplicationViewModel, assetID, fileDialog.fileUrl);
                            stackView.push(initialComponent, {backupModel: backupViewModelFromFile});
                        }
                    }

                    FileDialog {
                        id: exportFileDial
                        title: "Please choose folder"
                        folder: shortcuts.desktop
                        selectFolder: true
                        selectMultiple: false

                        onAccepted: {
                            backupViewModel.exportBackup(exportFileDial.fileUrl);
                        }
                    }
                }
            }
        }
    }

    Component {
        id: confirmComponent

        ColumnLayout {
            Layout.fillHeight: true
            Layout.fillWidth: true
            spacing: 15

            property var restoreBackupModel

            XSNLabel {
                Layout.fillWidth: true
                Layout.fillHeight: true
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                Layout.topMargin: 15
                font.family: mediumFont.name
                font.pixelSize: 22
                text: "Are you sure you want restore LN backup data?"
            }

            RowLayout {
                Layout.preferredHeight: 50
                Layout.alignment: Qt.AlignCenter
                spacing: 32

                IntroButton {
                    Layout.preferredWidth: 110
                    Layout.preferredHeight: 50
                    text: qsTr("Cancel")
                    onClicked: {
                        stackView.pop();
                    }
                }

                IntroButton {
                    Layout.preferredWidth: 110
                    Layout.preferredHeight: 50
                    font.family: mediumFont.name
                    text: qsTr("Confirm")
                    onClicked: {
                        restoreBackupModel.restoreFromBackup();
                        stackView.push("qrc:/Components/PopupProgressBarComponent.qml", {progressBarMsg: "Restoring data ..."})
                    }
                }
            }
        }
    }
}
