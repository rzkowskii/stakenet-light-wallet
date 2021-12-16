import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2

import "../Components"
import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

ListView {
    id: listView
    clip: true
    spacing: 10
    boundsBehavior: Flickable.StopAtBounds

    signal assetIsActiveToogled(var id)

    delegate: RowLayout {
        Layout.leftMargin: 20
        Layout.rightMargin: 20
        Layout.topMargin: 30
        Layout.bottomMargin: 30
        width: listView.width
        height: 110
        spacing: 10

        Item {
            Layout.preferredWidth: 60
            Layout.fillHeight: true

            RoundCheckbox {
                id: checkbox
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.right
                enabled: !model.isAlwaysActive
                checked: model.isActive === "1"

                PointingCursorMouseArea {
                    onClicked: assetIsActiveToogled(model.id)

                    onEntered:  {
                        checkbox.scale = scale * 1.1
                    }
                    onExited: {
                        checkbox.scale = scale
                    }
                }
            }
        }

        Rectangle {
            Layout.fillHeight: true
            Layout.fillWidth: true
            color: SkinColors.settingsAssetsBackground

            RowLayout {
                anchors.fill: parent
                anchors.rightMargin: 10
                spacing: 0

                Rectangle {
                    Layout.preferredWidth: 200
                    Layout.fillHeight: true
                    color: model.isActive === "1" ?  SkinColors.settingsActiveAsset :  "transparent"

                    PointingCursorMouseArea {
                        enabled: !model.isAlwaysActive
                        onClicked: assetIsActiveToogled(model.id)
                    }

                    RowLayout {
                        anchors.fill: parent
                        spacing: 10

                        Item {
                            Layout.preferredWidth: 80
                            Layout.fillHeight: true
                            Layout.alignment: Qt.AlignVCenter

                            Image {
                                anchors.centerIn: parent
                                sourceSize: Qt.size(35, 40)
                                source: "qrc:/images/ICON_%1.svg" .arg(model.name)
                            }
                        }

                        Column {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignVCenter

                            XSNLabel {
                                text: model.name
                                color: SkinColors.mainText
                                font.pixelSize: 14
                                font.family: fontRegular.name
                                font.bold: true
                                elide: Text.ElideRight
                                width: parent.width
                            }

                            XSNLabel {
                                text: model.symbol
                                color: SkinColors.mainText
                                font.pixelSize: 14
                                font.family: fontRegular.name
                                elide: Text.ElideRight
                                width: parent.width
                            }

                            XSNLabel {
                                text: "Always Active"
                                color: SkinColors.mainText
                                font.pixelSize: 12
                                visible: isAlwaysActive
                                font.family: fontRegular.name
                            }
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.alignment: Qt.AlignVCenter
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20

                    XSNLabel {
                        width: parent.width
                        anchors.centerIn: parent
                        text: model.description
                        fontSizeMode: Text.Fit;
                        minimumPixelSize: 12;
                        color: "white"
                        font.pixelSize: 14
                        font.family: fontRegular.name
                        wrapMode: Text.WordWrap
                        maximumLineCount: 4
                        elide: Text.ElideRight
                    }
                }

                ColumnLayout {
                    Layout.preferredWidth: 120
                    Layout.alignment: Qt.AlignVCenter
                    Layout.fillHeight: true
                    spacing: 10

                    RowLayout {
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignVCenter
                        spacing: 15

                        ReferenceButton {
                            Layout.preferredHeight: 40
                            Layout.preferredWidth: 40
                            source: "qrc:/images/link.png"
                            color: SkinColors.settingsShareIcons
                            radius: 30
                            visible: model.officialLink !== ""
                            onClicked: Qt.openUrlExternally(model.officialLink);
                        }

                        ReferenceButton {
                            Layout.preferredHeight: 40
                            Layout.preferredWidth: 40
                            source: "qrc:/images/reddit.png"
                            color: SkinColors.settingsShareIcons
                            radius: 20
                            visible: model.redditLink !== ""
                            onClicked: Qt.openUrlExternally(model.redditLink);
                        }

                        ReferenceButton {
                            Layout.preferredHeight: 40
                            Layout.preferredWidth: 40
                            source: "qrc:/images/asset_settings.png"
                            color: SkinColors.settingsShareIcons
                            radius: 30
                            enabled: model.isActive === "1"
                            visible: false
                            onClicked: openLNChannelsFeeDialog({assetID : model.id})
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.alignment: Qt.AlignVCenter
                        spacing: 15
                        visible:  model.twitterLink !== "" || model.telegramLink !== ""

                        ReferenceButton {
                            Layout.preferredWidth: 40
                            Layout.preferredHeight: 40
                            source: "qrc:/images/twitter_icon.png"
                            color: SkinColors.settingsShareIcons
                            radius: 20
                            visible: model.twitterLink !== ""
                            onClicked: Qt.openUrlExternally(model.twitterLink);
                        }

                        ReferenceButton {
                            Layout.preferredWidth: 40
                            Layout.preferredHeight: 40
                            source: "qrc:/images/telegram_icon.png"
                            color: SkinColors.settingsShareIcons
                            radius: 20
                            visible: model.telegramLink !== ""
                            onClicked: Qt.openUrlExternally(model.telegramLink);
                        }
                    }
                }
            }
        }
    }
}
