import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0

import com.xsn.viewmodels 1.0
import com.xsn.models 1.0
import com.xsn.utils 1.0

import "../Components"
import "../Popups"
import "../Pages"

RowLayout {
    spacing: 10

    FontLoader {
        id: regularFont
        source: "qrc:/Rubik-Regular.ttf"
    }

    Component {
        id: apSettingsComponent
        APSettingsPopup {}
    }

    Image {
        Layout.fillHeight: true
        sourceSize: Qt.size(40, 40)
        source: "qrc:/images/ic_lightning_rental.svg"
    }

    Column {
        Layout.preferredWidth: 120
        Layout.fillHeight: true
        spacing: 4

        XSNLabel {
            font.pixelSize: 12
            color: SkinColors.menuItemText
            anchors.horizontalCenter: parent.horizontalCenter
            text: "Can Send"
            font.family: regularFont.name
        }

        XSNLabel {
            id: canSendLbl
            font.pixelSize: 12
            text: "%1 %2".arg(
                      isActiveLightning ? Utils.formatBalance(
                                              paymentNodeViewModel.stateModel.nodeLocalRemoteBalances.allLocalBalance) : "0.0").arg(
                      symbol)
            anchors.horizontalCenter: parent.horizontalCenter
            font.family: regularFont.name
            color: "#dee0ff"
        }
    }

    Rectangle {
        Layout.preferredWidth: 2
        Layout.fillHeight: true
        color: SkinColors.popupFieldBorder
    }

    Column {
        Layout.preferredWidth: 120
        Layout.fillHeight: true
        spacing: 4

        XSNLabel {
            font.pixelSize: 12
            color: SkinColors.menuItemText
            anchors.horizontalCenter: parent.horizontalCenter
            text: "Can Receive"
            font.family: regularFont.name
        }

        XSNLabel {
            id: canReceiveLbl
            font.pixelSize: 12
            text: "%1 %2".arg(
                      isActiveLightning ? Utils.formatBalance(
                                              paymentNodeViewModel.stateModel.nodeLocalRemoteBalances.allRemoteBalance) : "0.0").arg(
                      symbol)
            anchors.horizontalCenter: parent.horizontalCenter
            font.family: regularFont.name
            color: "#dee0ff"
        }
    }

    Rectangle {
        Layout.preferredWidth: 2
        Layout.fillHeight: true
        color: SkinColors.popupFieldBorder
        visible: paymentNodeViewModel.stateModel.identifier.length !== 0
    }

    Column {
        Layout.preferredWidth: 120
        Layout.fillHeight: true
        spacing: 4
        visible: paymentNodeViewModel.stateModel.identifier.length !== 0

        XSNLabel {
            font.pixelSize: 12
            color: SkinColors.menuItemText
            text: "Public key"
            font.family: regularFont.name
        }

        Row {
            spacing: 5

            XSNLabel {
                id: pubKeyAddress
                font.pixelSize: 12
                text: paymentNodeViewModel.stateModel.identifier
                width: 180
                font.family: regularFont.name
                elide: Text.ElideRight
                color: "#dee0ff"
            }

            Image {
                anchors.verticalCenter: parent.verticalCenter
                sourceSize: Qt.size(15, 15)
                source: "qrc:/images/COPY ICONS.png"

                PointingCursorMouseArea {
                    anchors.fill: parent
                    onClicked: {
                        Clipboard.setText(pubKeyAddress.text)
                        showBubblePopup("Copied")
                    }
                }
            }
        }
    }

    Item {
        Layout.fillHeight: true
        Layout.fillWidth: true
    }

    ChannelRentalButton {
        Layout.preferredHeight: 32
        Layout.preferredWidth: 115
        visible: isActiveLightning
                 && paymentNodeViewModel.stateModel.nodeType === Enums.PaymentNodeType.Lnd
        onClicked: {
            var asset = walletAssetModel.get(
                        walletAssetModel.getInitial(assetID))
            if (allowOpenChannel()) {
                var popup = openChannelRentalDialog({
                                                        "assetID": assetID,
                                                        "symbol": asset.symbol,
                                                        "minLndCapacity": asset.minLndCapacity
                                                    })
            }
        }
    }

    AddChannelButton {
        id: addChannelRec
        Layout.preferredWidth: isActiveLightning ? 105 : 115
        Layout.preferredHeight: 32
        text: isActiveLightning ? "New channel" : "Open %1".arg(
                                                         paymentNodeViewModel.type === Enums.PaymentNodeType.Lnd ? "Lightning" : "Connext")
        onClicked: {
            if (isActiveLightning) {
                if (allowOpenChannel()) {
                    var popup = openChannelDialog({
                                                      "assetID": assetID
                                                  })
                }
            } else {
                ApplicationViewModel.paymentNodeViewModel.changeNodeActivity(
                            [], [assetID])
            }
        }
    }

    FadedRectangle {
        id: settingsRec
        Layout.preferredHeight: 32
        Layout.preferredWidth: 32
        activeStateColor: SkinColors.headerBackground
        inactiveStateColor: SkinColors.secondaryBackground
        radius: 16

        Image {
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            source: "qrc:/images/ic_ln_settings.svg"
            sourceSize: Qt.size(16, 16)
        }

        PointingCursorMouseArea {
            anchors.fill: parent
            onClicked: {
                contextMenu.open()
            }
            onEntered: {
                settingsRec.startFade()
            }
            onExited: {
                settingsRec.stopFade()
            }
        }

        Menu {
            id: contextMenu
            x: -contextMenu.width + 15
            y: 40
            width: 220
            height: 45

            Action {
                icon.source: contextMenu.currentIndex == 1 ? "qrc:/images/backup_hover.png" : "qrc:/images/backup_default.png"
                icon.height: 15
                icon.width: 15
                text: paymentNodeViewModel.type
                      === Enums.PaymentNodeType.Lnd ? qsTr("Backup") : qsTr(
                                                          "Restore channel")
                enabled: paymentNodeViewModel.stateModel.identifier.length > 0
                onTriggered: {

                    if (paymentNodeViewModel.type === Enums.PaymentNodeType.Lnd) {

                        openDialog(lnBackupsDialogComponent, {
                                       "assetID": assetID,
                                       "currentName": assetName,
                                       "currentSymbol": symbol
                                   })
                    } else {
                        paymentNodeViewModel.restoreChannel()
                    }
                }
            }

            AutopilotMenuItem {
                id: autopilotItem
                payNodeViewModel: paymentNodeViewModel
                visible: false
                height: 45
                width: parent.width
                onTriggered: openDialog(apSettingsComponent, {
                                            "assetID": assetID,
                                            "paymentNodeViewModel": paymentNodeViewModel
                                        })
            }

            delegate: MenuItemDelegate {
                id: menuItem
                width: 220
                height: 45
            }

            background: Rectangle {
                color: SkinColors.menuBackground
                border.color: SkinColors.popupFieldBorder
                radius: 5
            }
        }
    }
}
