import QtQuick 2.1
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.0

import "../Components"

import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0
import com.xsn.models 1.0

ActionDialog {
    id: root
    width: 600
    height: 465

    property var model
    property var channel: model.get(channelOutpoint)
    property string channelOutpoint: ""
    property string statusText: ""
    property string statusColor: ""
    property string statusImage: ""
    property string expiresAtTime: ""

    property int assetID: 0
    property string currentSymbol: ""
    property int confirmationsForApproved: 0
    property bool isPending: statusText === "Pending"
    property int bitcoinID: 0
    property bool showConfirmationLink: channel.type !== PaymentChannelsListModel.ChannelType.Closing && channel.confirmations > 0

    signal openUrlRequested(string channelId)

    function changeChannel(statusText, statusColor, statusImage, channel)
    {
        root.statusText = statusText;
        root.statusColor = statusColor
        root.statusImage = statusImage
        root.channel = channel
    }

    FontLoader { id: regularFont; source: "qrc:/Rubik-Regular.ttf" }
    FontLoader { id: lightFont; source: "qrc:/Rubik-Light.ttf" }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 15
        anchors.rightMargin: 15
        anchors.topMargin: 15
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 35

            XSNLabel {
                font.family: mediumFont.name
                font.pixelSize: 18
                text: "%1channel details".arg(channel.isRentingChannel ? "rental " : "")
                color: SkinColors.mainText
                font.capitalization: Font.Capitalize
            }

            Item {
                Layout.fillWidth: true
            }

            CloseButton {
                Layout.preferredHeight: 20
                Layout.preferredWidth: 20
                Layout.alignment: Qt.AlignRight | Qt.AlignTop
                onClicked: root.close()
            }
        }

        Rectangle {
            Layout.preferredHeight: 1
            Layout.fillWidth: true
            color: SkinColors.popupFieldBorder
        }

        RowLayout {
            Layout.preferredHeight: 45
            Layout.fillWidth: true
            spacing: 10

            Item {
                Layout.fillHeight: true
                Layout.preferredWidth: 220

                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 5

                    SecondaryLabel {
                        text: qsTr("Capacity:")
                        font.pixelSize: 12
                    }

                    Row {
                        spacing: 0

                        SelectedText {
                            text: channel !== null ? "%1 %2".arg(Utils.formatBalance(channel.capacity)).arg(currentSymbol) : ""
                            font.pixelSize: 13
                            font.family: lightFont.name
                            color: SkinColors.mainText
                        }

                        SelectedText {
                            text: channel !== null ? " ( = %1 %2 )".arg(ApplicationViewModel.localCurrencyViewModel.convert(assetID, Utils.formatBalance(channel.capacity))).arg(ApplicationViewModel.localCurrencyViewModel.currentCurrencyCode) : ""
                            font.pixelSize: 13
                            color: SkinColors.secondaryText
                            font.family: lightFont.name
                        }
                    }
                }
            }

            Rectangle {
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                color: SkinColors.popupFieldBorder
                visible: !isPending
            }

            Column {
                Layout.fillWidth: true
                Layout.leftMargin: 10
                spacing: 5
                visible: !isPending

                SecondaryLabel {
                    text: qsTr("Channel Id:")
                    font.pixelSize: 12
                }

                SelectedText {
                    text: channel !== null ? channel.channelID : ""
                    font.pixelSize: 13
                    font.family: lightFont.name
                    color: SkinColors.mainText
                }
            }
        }

        Rectangle {
            Layout.preferredHeight: 1
            Layout.fillWidth: true
            color: SkinColors.popupFieldBorder
        }

        RowLayout {
            Layout.preferredHeight: 45
            Layout.fillWidth: true
            spacing: 10

            Column {
                Layout.preferredWidth: 220
                spacing: 5

                SecondaryLabel {
                    text: qsTr("Can send:")
                    font.pixelSize: 12
                }

                SelectedText {
                    text: channel !== null ? "%1 %2".arg(Utils.formatBalance(channel.localBalance)).arg(currentSymbol) : ""
                    font.pixelSize: 13
                    font.family: lightFont.name
                    color: SkinColors.mainText
                }
            }

            Rectangle {
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                color: SkinColors.popupFieldBorder
            }

            Column {
                Layout.fillWidth: true
                Layout.leftMargin: 10
                spacing: 5

                SecondaryLabel {
                    text: qsTr("Total sent:")
                    font.pixelSize: 12
                }

                SelectedText {
                    text: channel !== null ? (!isPending ? "%1 %2".arg(Utils.formatBalance(channel.totalSent)).arg(currentSymbol) : "0") : ""
                    font.pixelSize: 13
                    font.family: lightFont.name
                    color: SkinColors.mainText
                }
            }
        }

        Rectangle {
            Layout.preferredHeight: 1
            Layout.fillWidth: true
            color: SkinColors.popupFieldBorder
        }

        RowLayout {
            Layout.preferredHeight: 45
            Layout.fillWidth: true
            spacing: 10

            Column {
                Layout.preferredWidth: 220
                spacing: 5

                SecondaryLabel {
                    text: qsTr("Can receive:")
                    font.pixelSize: 12
                }

                SelectedText {
                    text: channel !== null ? "%1 %2".arg(Utils.formatBalance(channel.remoteBalance)).arg(currentSymbol) : ""
                    font.pixelSize: 13
                    font.family: lightFont.name
                    color: SkinColors.mainText
                }
            }

            Rectangle {
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                color: SkinColors.popupFieldBorder
            }

            Column {
                Layout.fillWidth: true
                Layout.leftMargin: 10
                spacing: 5

                SecondaryLabel {
                    text: qsTr("Total received:")
                    font.pixelSize: 12
                }

                SelectedText {
                    text: channel !== null ? (!isPending ? "%1 %2".arg(Utils.formatBalance(channel.totalReceive)).arg(currentSymbol) : "0") : ""
                    font.pixelSize: 13
                    font.family: lightFont.name
                    color: SkinColors.mainText
                }
            }
        }

        Rectangle {
            Layout.preferredHeight: 1
            Layout.fillWidth: true
            color: SkinColors.popupFieldBorder
        }

        RowLayout {
            Layout.preferredHeight: 45
            Layout.fillWidth: true
            spacing: 10

            Column {
                Layout.preferredWidth: 220
                spacing: 5

                SecondaryLabel {
                    text: qsTr("Rental time remaining:")
                    font.pixelSize: 12
                }

                RowLayout {
                    spacing: 7

                    Image {
                        source: "qrc:/images/ADD_BUTTON.png"
                        visible: channel !== null && channel.isRentingChannel

                        PointingCursorMouseArea {
                            enabled: channel.type === PaymentChannelsListModel.ChannelType.Active
                            onClicked: {
                                var popup = openDialog(extenderRentalComponent, {assetID : assetID, capacity: channel.remoteBalance, symbol : currentSymbol, fundingTxOutpoint: channel.channelOutpoint});
                            }
                        }
                    }

                    SelectedText {
                        font.pixelSize: channel !== null && channel.isRentingChannel ? 13 : 16
                        text: channel !== null ? channel.isRentingChannel ? Utils.formatTime(channel.expiresAtTime, true) : "∞" : 0
                        horizontalAlignment: Text.AlignLeft
                        font.family: lightFont.name
                        color: SkinColors.mainText
                    }
                }
            }

            Rectangle {
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                color: SkinColors.popupFieldBorder
            }

            Column {
                Layout.fillWidth: true

                SecondaryLabel {
                    text: qsTr("Remote public key:")
                    anchors.leftMargin: 10
                    anchors.left: parent.left
                    font.pixelSize: 12
                }

                Item {
                    width: parent.width
                    height: 30

                    CopiedField {
                        width: parent.width * 0.9
                        height: 30
                        text: channel.remotePubKey
                        bgColor: SkinColors.mainBackground
                    }
                }
            }
        }

        Rectangle {
            Layout.preferredHeight: 1
            Layout.fillWidth: true
            color: SkinColors.popupFieldBorder
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 85
            spacing: 10

            Column {
                Layout.preferredWidth: 220
                spacing: 5


                SecondaryLabel {
                    text: qsTr("Date:")
                    font.pixelSize: 12
                    visible: channel.confirmations > 0
                }

                SelectedText {
                    text: channel !== null ? Utils.formatDate(channel.channelDate) : ""
                    font.pixelSize: 13
                    font.family: lightFont.name
                    color: SkinColors.mainText
                    visible: channel.confirmations > 0
                }

                SecondaryLabel {
                    text: qsTr("Status:")
                    font.pixelSize: 12
                }

                Row {
                    spacing: 5

                    Image {
                        sourceSize: Qt.size(20, 20)
                        source: statusImage
                    }

                    SelectedText {
                        anchors.verticalCenter: parent.verticalCenter
                        text: statusText
                        color: statusColor
                        font.pixelSize: 12
                        font.family: lightFont.name
                    }
                }

                XSNLabel {
                    height: 15
                    font.pixelSize: 12
                    font.family: lightFont.name
                    font.underline: showConfirmationLink
                    text: "Confirmations: %1/%2".arg(channel.type === PaymentChannelsListModel.ChannelType.Closing ? "0" : channel.confirmations)
                    .arg(channel.type === PaymentChannelsListModel.ChannelType.ForceClosing ? channel.details.maturity_blocks
                    : (channel.type === PaymentChannelsListModel.ChannelType.Closing ? "1" : confirmationsForApproved))
                    visible: channel.type !== PaymentChannelsListModel.ChannelType.Active && channel.type !== PaymentChannelsListModel.ChannelType.Inactive
                    anchors.bottomMargin: 5

                    PointingCursorMouseArea {
                        cursorShape: showConfirmationLink ? Qt.PointingHandCursor : Qt.ArrowCursor
                        enabled: showConfirmationLink
                        onClicked: {
                            openUrlRequested(channel.fundingTxId);
                        }
                    }
                }
            }

            Rectangle {
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                color: SkinColors.popupFieldBorder
            }

            Column {
                Layout.fillWidth: true

                SecondaryLabel {
                    anchors.leftMargin: 10
                    anchors.left: parent.left
                    text: qsTr("Funding transaction Id:")
                    font.pixelSize: 12
                }

                Item {
                    width: parent.width
                    height: 30

                    CopiedField {
                        width: parent.width * 0.9
                        height: 30
                        text: channel !== null ? channel.fundingTxId : ""
                        bgColor: SkinColors.mainBackground
                    }
                }
            }
        }

        Rectangle {
            Layout.preferredHeight: 1
            Layout.fillWidth: true
            color: SkinColors.popupFieldBorder
        }

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 45
            Layout.alignment: Qt.AlignRight


            XSNLabel {
                visible: channel.type === PaymentChannelsListModel.ChannelType.Active && (channel.confirmations >= 0 || assetID === bitcoinID)
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                text: "<a href=\"dummy\">Open in explorer</a>"
                font.underline: true
                font.pixelSize: 13
                font.family: mediumFont.name
                linkColor: SkinColors.menuBackgroundGradientFirst
                font.capitalization: Font.AllUppercase
                font.weight: Font.Medium

                PointingCursorMouseArea{
                    onClicked: {
                        openUrlRequested(channel.fundingTxId);
                    }
                }
            }
        }
    }
}
