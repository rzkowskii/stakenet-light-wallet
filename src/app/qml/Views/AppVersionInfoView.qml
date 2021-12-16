import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls.Styles 1.4

import "../Components"
import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

ColumnLayout {
    property string infoText: ""
    spacing: 40

    FontLoader {
        id: regularFont
        source: "qrc:/Rubik-Regular.ttf"
    }

    Connections {
        target: AppUpdater
        function onAlreadyAtLatestVersion() {
            infoText = "On latest version"
        }

        function onUpdaterStateChanged() {
            if (AppUpdater.updaterState === AppUpdater.UpdaterState.Ready) {
                infoText = "A new version of Stakenet DEX is available. \nThe app will now close and restart, please do not manually start the app."
            } else if (AppUpdater.updaterState === AppUpdater.UpdaterState.Checking) {
                infoText = "Checking for updates..."
            } else if (AppUpdater.updaterState === AppUpdater.UpdaterState.ExistNew) {
                infoText = "A new version of Stakenet DEX is available"
            } else if (AppUpdater.updaterState === AppUpdater.UpdaterState.Downloading) {
                infoText = "Downloading update"
            } else if (AppUpdater.updaterState === AppUpdater.UpdaterState.CheckingFailed) {
                infoText = "Check for updates failed. Please verify connection and try again."
            } else if (AppUpdater.updaterState === AppUpdater.UpdaterState.DownloadingFailed) {
                infoText = "Downloading updates failed. Please verify connection and try again."
            }
            else {
                infoText = ""
            }
        }

        function onDownloadProgress(progress) {
            progressBar.value = progress
        }
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 5

        XSNLabel {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            text: "Software Version"
            font.pixelSize: 24
            Layout.alignment: Qt.AlignLeft
            font.weight: Font.Light
        }

        Repeater {
            model: [{
                    "title": "Application version",
                    "content": appVersion
                }, {
                    "title": "Application environment",
                    "content": isStaging ? "Staging" : "Production"
                }, {
                    "title": "Application build hash",
                    "content": buildHash
                }, {
                    "title": "Client ID",
                    "content": ApplicationViewModel.clientId
                }]

            delegate: RowLayout {
                Layout.preferredHeight: 20
                Layout.fillWidth: true
                spacing: 15
                property bool isCurrentItem: currentIndex == index

                XSNLabel {
                    Layout.preferredWidth: 180
                    font.pixelSize: 14
                    Layout.alignment: Qt.AlignVCenter
                    text: modelData.title
                    font.family: regularFont.name
                    font.capitalization: Font.Capitalize
                }

                XSNLabel {
                    Layout.preferredWidth: 5
                    font.pixelSize: 14
                    Layout.alignment: Qt.AlignVCenter
                    text: ":"
                    font.family: regularFont.name
                }

                FadedText {
                    id: contentText
                    Layout.fillWidth: true
                    font.pixelSize: 14
                    Layout.alignment: Qt.AlignVCenter
                    text: modelData.content
                    color: SkinColors.secondaryText
                    font.family: regularFont.name
                    isCurrentItem: isCurrentItem
                    font.capitalization: Font.MixedCase

                    PointingCursorMouseArea {
                        onClicked: {
                            Clipboard.setText(modelData.content)
                            showBubblePopup("Copied")
                        }
                        onEntered: contentText.startFade()
                        onExited: contentText.stopFade()
                    }
                }
            }
        }
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 15

        XSNLabel {
            text: "Updates"
            font.family: regularFont.name
            color: SkinColors.secondaryText
            font.pixelSize: 14
        }

        Item {
            Layout.preferredHeight: 80
            Layout.fillWidth: true

            Rectangle {
                anchors.fill: parent
                color: SkinColors.secondaryBackground
                radius: 5
            }

            RowLayout {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 20

                Image {
                    source: "qrc:/images/IC_UPDATES_SETTINGS.svg"
                }

                ColumnLayout {
                    Layout.alignment: Qt.AlignVCenter
                    Layout.fillWidth: true
                    visible: infoText != ""
                    spacing: 7

                    XSNLabel {
                        Layout.fillWidth: true
                        text: infoText
                        font.family: regularFont.name
                        font.pixelSize: 14
                    }

                    XSNLabel {
                        Layout.fillWidth: true
                        color: SkinColors.secondaryText
                        text: "Version : %1".arg(AppUpdater.readyVersionStr)
                        font.family: regularFont.name
                        font.pixelSize: 12
                        visible: AppUpdater.updaterState === AppUpdater.UpdaterState.Ready
                    }

                    ProgressBar {
                        id: progressBar
                        Layout.alignment: Qt.AlignCenter
                        Layout.fillWidth: true
                        Layout.preferredHeight: 6
                        visible: AppUpdater.updaterState === AppUpdater.UpdaterState.Downloading

                        background: Rectangle {
                            anchors.fill: parent
                            color: "white"
                            radius: 3
                        }

                        contentItem: Item {
                            anchors.fill: parent

                            Rectangle {
                                width: progressBar.visualPosition * progressBar.width
                                height: progressBar.height
                                radius: 3
                                color: SkinColors.menuBackgroundGradienRightLine
                            }
                        }
                    }
                }

                IntroButton {
                    Layout.preferredWidth: 160
                    Layout.preferredHeight: 32
                    visible: AppUpdater.updaterState !== AppUpdater.UpdaterState.Ready
                    text: AppUpdater.updaterState === AppUpdater.UpdaterState.ExistNew || AppUpdater.updaterState === AppUpdater.UpdaterState.DownloadingFailed ? "Download" : "Check for updates"
                    onClicked: {
                        if(AppUpdater.updaterState === AppUpdater.UpdaterState.ExistNew || AppUpdater.updaterState === AppUpdater.UpdaterState.DownloadingFailed){
                            AppUpdater.downloadUpdate()
                        }
                        else {
                            AppUpdater.checkForUpdates()
                        }
                    }
                }

                IntroButton {
                    visible: AppUpdater.updaterState === AppUpdater.UpdaterState.Ready
                    Layout.preferredWidth: 160
                    Layout.preferredHeight: 32
                    text: "Update and restart"
                    onClicked: {
                        AppUpdater.startUpdating()
                        infoText = ""
                    }
                }
            }
        }
    }

    ColumnLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        spacing: 15

        XSNLabel {
            Layout.fillWidth: true
            text: "Update history"
            font.family: regularFont.name
            color: SkinColors.secondaryText
            font.pixelSize: 14
        }

        ListView {
            id: listView
            Layout.fillHeight: true
            Layout.fillWidth: true
            spacing: 5
            currentIndex: 0
            boundsBehavior: Flickable.StopAtBounds
            clip: true

            model: Utils.changeLog()

            delegate: Item {
                id: delegateItem
                width: listView.width
                height: ListView.isCurrentItem ? changelogText.contentHeight
                                                 + date.contentHeight + headerItem.contentHeight
                                                 + layout.spacing + 40 : 72
                clip: true

                FadedRectangle {
                    id: fadeBgr
                    anchors.fill: parent
                    activeStateColor: SkinColors.headerBackground
                    inactiveStateColor: SkinColors.secondaryBackground
                    radius: 5
                }

                RowLayout {
                    anchors.margins: 20
                    anchors.fill: parent
                    spacing: 0

                    ColumnLayout {
                        id: layout
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 20

                        Column {
                            spacing: delegateItem.ListView.isCurrentItem ? 5 : 3

                            XSNLabel {
                                id: headerItem
                                font.family: regularFont.name
                                font.pixelSize: 14
                                text: "Changelog for Version %1".arg(modelData.version)
                            }

                            XSNLabel {
                                id: date
                                font.family: regularFont.name
                                font.pixelSize: 12
                                text: "Released on : %1".arg(modelData.date)
                                color: SkinColors.secondaryText
                            }
                        }

                        XSNLabel {
                            id: changelogText
                            font.family: regularFont.name
                            font.pixelSize: 14
                            text: modelData.updateContent
                            visible: delegateItem.ListView.isCurrentItem
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                    }

                    Item {
                        Layout.alignment: delegateItem.ListView.isCurrentItem ? Qt.AlignTop : Qt.AlignVCenter
                        Layout.preferredHeight: 20
                        Layout.preferredWidth: 20

                        Image {
                            anchors.centerIn: parent
                            source: delegateItem.ListView.isCurrentItem ? "qrc:/images/ic_up.svg" : "qrc:/images/ic_down.svg"
                        }
                    }
                }

                PointingCursorMouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (listView.currentIndex == index) {
                            listView.currentIndex = -1
                        } else {
                            listView.currentIndex = index
                        }
                    }
                    onEntered: fadeBgr.startFade()
                    onExited: fadeBgr.stopFade()
                }
            }
        }
    }
}
