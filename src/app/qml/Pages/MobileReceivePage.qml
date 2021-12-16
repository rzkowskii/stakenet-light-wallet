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

    property int assetID: 0
    property int currentIndex: -1
    property string currentSymbol: assetModel.get(currentIndex).symbol
    property string currentBalance: Utils.formatBalance(assetModel.get(currentIndex).balance)
    property string currentName: assetModel.get(currentIndex).name
    property string currentColor:assetModel.get(currentIndex).color
    property var receivingAddresses: undefined

    FontLoader { id: regularFont; source: "qrc:/Rubik-Regular.ttf" }
    FontLoader { id: lightFont; source: "qrc:/Rubik-Light.ttf" }


    background: Rectangle {
        color: "transparent"
    }

    Component.onCompleted: {
        ApplicationViewModel.walletViewModel.requestAllKnownAddressesById(assetID);
    }

    Connections {
        target: ApplicationViewModel.walletViewModel
        function onAllKnownAddressByIdGenerated(addresses) {
            receivingAddresses = addresses
        }
    }

    WalletAssetsListModel {
        id: assetModel

        Component.onCompleted: {
            initialize(ApplicationViewModel);
            root.currentIndex = getInitial(assetID)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 30
        anchors.bottomMargin: 40
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        spacing: 35

        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            spacing: 20

            MobileTitle {
                Layout.alignment: Qt.AlignCenter
                text: "send %1" .arg(currentName)
            }

            Image {
                Layout.alignment: Qt.AlignCenter
                source: currentName !== "" ? "qrc:/images/ICON_%1.svg".arg(currentName): ""
                sourceSize: Qt.size(43, 49)
            }

            SecondaryLabel {
                Layout.alignment: Qt.AlignHCenter
                text: "Your %1 Address:" .arg(currentName)
                font.capitalization: Font.Capitalize
                font.pixelSize: 12
                font.family: regularFont.name
            }
        }

        Rectangle {
            Layout.preferredHeight: 120
            Layout.preferredWidth: 120
            Layout.alignment: Qt.AlignHCenter
            radius: 8

            Image {
                anchors.centerIn: parent
                source: "qrc:/images/QRCode_Example.png"
                sourceSize: Qt.size(110, 110)
            }
        }

        CustomizedComboBox {
            id: addressesComboBox
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredHeight: 30
            Layout.fillWidth: true
            font.pixelSize: 12
            content.color: currentColor
            leftPadding: 13

            font.family: lightFont.name
            background: Rectangle {
                color: "transparent"
            }

            model: receivingAddresses
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 50

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignHCenter

                MobileIconButton {
                    anchors.centerIn: parent
                    color: SkinColors.mobileSecondaryBackground
                    image.source: "qrc:/images/ic_clone.png"
                    onClicked: {
                        Clipboard.setText(addressesComboBox.currentText);
                        mainWindow.showBubblePopup("Copied");
                    }
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignHCenter

                MobileIconButton {
                    anchors.centerIn: parent
                    color: SkinColors.mobileSecondaryBackground
                    image.source: "qrc:/images/ic_print.png"
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignHCenter

                MobileIconButton {
                    anchors.centerIn: parent
                    color: SkinColors.mobileSecondaryBackground
                    image.source: "qrc:/images/ic_email.png"
                }
            }

        }

        MobileActionButton {
            Layout.leftMargin: 6
            Layout.rightMargin: 6
            Layout.preferredHeight: 41
            Layout.fillWidth: true
            buttonColor: currentColor
            buttonText: "Close"
            onClicked: navigateBack()
        }
    }
}
