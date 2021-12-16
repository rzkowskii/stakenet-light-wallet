import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import "../Components"
import "../Views"

import com.xsn.viewmodels 1.0
import com.xsn.models 1.0
import com.xsn.utils 1.0

Page {
    id: root

    background: Rectangle {
        color: "transparent"
    }

    FontLoader { id: regularFont; source: "qrc:/Rubik-Regular.ttf" }

    ColumnLayout {
        id: layout
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.topMargin: 30
        anchors.bottomMargin: 20
        spacing: 40

        MobileTitle {
            Layout.alignment: Qt.AlignHCenter
            text: "Assets"
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 10

            SecondaryLabel {
                Layout.leftMargin: 13
                font.family: regularFont.name
                font.pixelSize: 12
                text: "Active (%1/%2)".arg(assetModel.activeAssetsCount) .arg(assetModel.count)
            }

            ListView {
                Layout.fillHeight: true
                Layout.fillWidth: true
                spacing: 6
                model: WalletAssetsListModel {
                    id: assetModel
                    Component.onCompleted: initialize(ApplicationViewModel)
                }

                delegate: Item  {
                    width: parent.width
                    height: 57

                    Rectangle {
                        anchors.fill: parent
                        radius: 8
                        color: SkinColors.mobileSettingsSecondaryBackground
                        opacity: 0.6
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 13
                        spacing: 13

                        Image {
                            Layout.alignment: Qt.AlignVCenter
                            source: "qrc:/images/ICON_%1.svg" .arg(model.name)
                            sourceSize: Qt.size(29, 33)
                        }

                        Row {
                            Layout.alignment: Qt.AlignVCenter
                            spacing: 5

                            XSNLabel {
                                text: model.name
                                color: SkinColors.mainText
                                font.pixelSize: 14
                            }

                            XSNLabel {
                                text: "- %1".arg(model.symbol)
                                color: SkinColors.mainText
                                font.capitalization: Font.AllUppercase
                                font.pixelSize: 14
                            }
                        }

                        Item {
                            Layout.fillWidth: true
                        }

                        SwitchComponent {
                            id: switchItem
                            checked: model.isActive === "1"
                            enabled: !model.isAlwaysActive
                            Layout.alignment: Qt.AlignVCenter
                            onClicked:{
                                assetModel.toogleAssetActiveState(model.id)
                            }
                        }
                    }
                }
            }
        }

        MobileFooter {
            Layout.leftMargin: 13
            Layout.preferredHeight: 40
            leftButton.text: "back"
            onLeftButtonClicked: {
                navigateBack()
            }
        }
    }
}
