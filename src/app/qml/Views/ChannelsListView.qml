import QtQuick 2.6
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12
import QtGraphicalEffects 1.0

import "../Components"
import "../Pages"
import "../Popups"

import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0
import com.xsn.models 1.0

ListView {
    id: channelsListView

    boundsBehavior: Flickable.StopAtBounds
    clip: true
    highlightFollowsCurrentItem: true
    spacing: 0

    property PaymentNodeViewModel paymentNodeViewModel

    signal openInExplorerRequested(string txid)
    signal deleteChannelRequested(string channelOutpoint, int channelType, int channelCsvDelay)
    property string currentAssetSymbol: ""
    property int currentConfirmationsForApproved: 0
    property int currentAssetID: 0

    property var dialog

    Connections {
        target: channelsListView.model

        function onDataChanged() {
            if (dialog) {
                var channel = model.get(dialog.channelOutpoint)
                dialog.changeChannel(statusText(channel.type),
                                     statusColor(channel.type),
                                     statusImage(channel.type), channel)
            }
        }
    }

    delegate: paymentNodeViewModel.type
              === Enums.PaymentNodeType.Lnd ? lndChannelComponent : connextChannelComponent

    Component {
        id: lndChannelComponent

        DelegateBackgroundItem {
            id: delegate
            property bool isCurrentItem: ListView.isCurrentItem
            width: channelsListView.width
            height: 40

            property int type: model.type
            property string channelID: model.channelID
            property string localBalance: model.localBalance
            property string remoteBalance: model.remoteBalance
            property string capacity: model.capacity
            property string channelOutpoint: model.channelOutpoint
            property int csvDelay: model.csvDelay
            property var details: model.details
            property string fundingTxId: model.fundingTxId
            property bool isRentingChannel: model.isRentingChannel
            property string expiresAtTime: model.expiresAtTime
            property string channelDate: model.channelDate
            property string formatTime: Utils.formatTime(model.expiresAtTime)

            onSelected: {
                dialog = openDialog(channelDetailsComponent, {
                                        "model": channelsListView.model,
                                        "channelOutpoint": delegate.channelOutpoint,
                                        "currentSymbol": channelsListView.currentAssetSymbol,
                                        "assetID": channelsListView.currentAssetID,
                                        "confirmationsForApproved": channelsListView.currentConfirmationsForApproved,
                                        "expiresAtTime": delegate.expiresAtTime,
                                        "statusText": statusText(delegate.type),
                                        "statusColor": statusColor(
                                                           delegate.type),
                                        "statusImage": statusImage(
                                                           delegate.type)
                                    })

                dialog.openUrlRequested.connect(openInExplorerRequested)
            }

            ToolTip {
                delay: 1000
                timeout: 5000
                visible: (delegate.mouseArea.containsMouse)
                         && (delegate.type === PaymentChannelsListModel.ChannelType.ForceClosing)
                text: "Remains %1 blocks until the commitment output can be swept.".arg(
                          delegate.details.blocks_til_maturity)
                enter: Transition {
                    NumberAnimation {
                        property: "opacity"
                        duration: 350
                        to: 1
                    }
                }
                exit: Transition {
                    NumberAnimation {
                        property: "opacity"
                        duration: 350
                        to: 0
                    }
                }
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 25
                anchors.rightMargin: 15
                spacing: 0

                XSNLabel {
                    Layout.preferredWidth: parent.width * 0.2
                    text: "Channel %1".arg(index + 1)
                    color: "#8387b4"
                    font.family: regularFont.name
                    font.pixelSize: 11
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                }

                XSNLabel {
                    Layout.preferredWidth: parent.width * 0.15
                    text: delegate.type !== PaymentChannelsListModel.ChannelType.Active
                          && delegate.type !== PaymentChannelsListModel.ChannelType.Inactive ? ("%1 %2/%3".arg(statusText(delegate.type)).arg(delegate.type === PaymentChannelsListModel.ChannelType.Closing ? "0" : model.confirmations).arg(delegate.type === PaymentChannelsListModel.ChannelType.ForceClosing ? delegate.details.maturity_blocks : (delegate.type === PaymentChannelsListModel.ChannelType.Closing ? "1" : currentConfirmationsForApproved))) : "%1".arg(statusText(delegate.type))
                    color: statusColor(delegate.type)
                    elide: Text.ElideRight
                    font.family: regularFont.name
                    font.pixelSize: 11
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                }

                XSNLabel {
                    id: canSend
                    Layout.preferredWidth: parent.width * 0.2
                    horizontalAlignment: Text.AlignLeft
                    font.pixelSize: 11
                    text: Utils.formatBalance(delegate.localBalance)
                    font.family: regularFont.name
                    color: "#8387b4"
                }

                XSNLabel {
                    id: canReceive
                    Layout.preferredWidth: parent.width * 0.22
                    horizontalAlignment: Text.AlignLeft
                    color: "#8387b4"
                    font.pixelSize: 11
                    text: Utils.formatBalance(delegate.remoteBalance)
                    font.family: regularFont.name
                }

                XSNLabel {
                    Layout.preferredWidth: parent.width * 0.33
                    horizontalAlignment: Text.AlignLeft
                    visible: delegate.isRentingChannel && formatTime !== ""
                    text: "%1 %2".arg(formatTime).arg(
                              formatTime === "Expired" ? "" : "left")
                    color: "#8387b4"
                    font.pixelSize: 11
                    font.family: regularFont.name
                }

                Item {
                    Layout.fillWidth: true
                }

                Image {
                    source: "qrc:/images/ic_close_channels.svg"
                    sourceSize: Qt.size(12, 12)
                    Layout.preferredWidth: 12

                    visible: paymentNodeViewModel.type === Enums.PaymentNodeType.Lnd

                    PointingCursorMouseArea {
                        id: detailsMouseArea
                        onClicked: {
                            if (simpleSwapInProcess) {
                                showBubblePopup(
                                            "Unable to close channel while a swap is in progress!")
                                return
                            }

                            if (delegate.type === PaymentChannelsListModel.ChannelType.Active
                                    || delegate.type
                                    === PaymentChannelsListModel.ChannelType.Inactive
                                    || delegate.type
                                    === PaymentChannelsListModel.ChannelType.Closing) {
                                deleteChannelRequested(
                                            delegate.channelOutpoint,
                                            delegate.type, delegate.csvDelay)
                            } else {
                                showBubblePopup(
                                            "You can not close this type of channel!")
                            }
                        }
                    }
                }
            }
        }
    }

    Component {
        id: connextChannelComponent

        DelegateBackgroundItem {
            id: delegate
            property bool isCurrentItem: ListView.isCurrentItem
            width: channelsListView.width
            height: 40

            property int type: model.type
            property string channelID: model.channelID
            property string localBalance: model.localBalance
            property string remoteBalance: model.remoteBalance
            property string capacity: model.capacity
            property int csvDelay: model.csvDelay
            property var details: model.details
            property string fundingTxId: model.fundingTxId
            property bool isRentingChannel: model.isRentingChannel
            property string expiresAtTime: model.expiresAtTime
            property string channelDate: model.channelDate
            property string formatTime: Utils.formatTime(model.expiresAtTime)

            onSelected: {
                dialog = openDialog(connextChannelDetailsComponent, {
                                        "model": channelsListView.model,
                                        "channelOutpoint": delegate.channelID,
                                        "currentSymbol": channelsListView.currentAssetSymbol,
                                        "assetID": channelsListView.currentAssetID,
                                        "confirmationsForApproved": channelsListView.currentConfirmationsForApproved,
                                        "expiresAtTime": delegate.expiresAtTime,
                                        "statusText": statusText(delegate.type),
                                        "statusColor": statusColor(
                                                           delegate.type),
                                        "statusImage": statusImage(
                                                           delegate.type)
                                    })

                dialog.openUrlRequested.connect(openInExplorerRequested)
            }

            ToolTip {
                delay: 1000
                timeout: 5000
                visible: (mouseArea.containsMouse)
                         && (delegate.type === PaymentChannelsListModel.ChannelType.ForceClosing)
                         && (paymentNodeViewModel.type === Enums.PaymentNodeType.Lnd)
                text: "Remains %1 blocks until the commitment output can be swept.".arg(
                          delegate.details.blocks_til_maturity)
                enter: Transition {
                    NumberAnimation {
                        property: "opacity"
                        duration: 350
                        to: 1
                    }
                }
                exit: Transition {
                    NumberAnimation {
                        property: "opacity"
                        duration: 350
                        to: 0
                    }
                }
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 25
                anchors.rightMargin: 15
                spacing: 0

                XSNLabel {
                    Layout.preferredWidth: parent.width * 0.2
                    text: "Channel %1".arg(index + 1)
                    color: "#8387b4"
                    font.family: regularFont.name
                    font.pixelSize: 11
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                }

                XSNLabel {
                    id: canSend
                    Layout.preferredWidth: parent.width * 0.2
                    horizontalAlignment: Text.AlignLeft
                    font.pixelSize: 11
                    text: Utils.formatBalance(delegate.localBalance)
                    font.family: regularFont.name
                    color: "#8387b4"
                }

                XSNLabel {
                    id: canReceive
                    Layout.preferredWidth: parent.width * 0.22
                    horizontalAlignment: Text.AlignLeft
                    color: "#8387b4"
                    font.pixelSize: 11
                    text: Utils.formatBalance(delegate.remoteBalance)
                    font.family: regularFont.name
                }

                Item {
                    Layout.fillWidth: true
                }

                XSNLabel {
                    id: channelMenuBtn
                    Layout.alignment: Qt.AlignVCenter
                    text: ". . ."
                    rotation: 90
                    font.pixelSize: 13
                    color: SkinColors.mainText

                    PointingCursorMouseArea {
                        anchors.fill: parent

                        onClicked: {
                            contextMenu.popup(channelMenuBtn)
                        }
                    }
                }

                Menu {
                    id: contextMenu
                    width: 130
                    height: 90

                    Action {
                        text: qsTr("Reconcile")
                        enabled: checkPaymentNodeState(payNodeViewModel.stateModel)
                        onTriggered: {
                            if (allowReconcileChannel()) {
                                paymentNodeViewModel.reconcileChannel(
                                            delegate.channelID)
                            }
                        }
                    }

                    Action {
                        text: qsTr("Deposit")
                        enabled: checkPaymentNodeState(payNodeViewModel.stateModel)
                        onTriggered: {
                            if (allowDepositChannel()) {
                                var popup = openDialog(depositDialog, {
                                                           "assetID": assetID,
                                                           "channelAddress": delegate.channelID
                                                       })
                            }
                        }
                    }
                    Action {
                        text: qsTr("Withdraw")
                        enabled: checkPaymentNodeState(payNodeViewModel.stateModel)
                        onTriggered: {
                            if (allowWithdrawChannel()) {
                                var popup = openDialog(withdrawDialog, {
                                                           "assetID": assetID,
                                                           "channelAddress": delegate.channelID,
                                                           "paymentNodeViewModel": paymentNodeViewModel
                                                       })
                            }
                        }
                    }

                    delegate: MenuItemDelegate {
                        id: menuItem
                    }

                    background: Rectangle {
                        implicitWidth: 180
                        implicitHeight: 30
                        color: SkinColors.menuBackground
                        border.color: SkinColors.popupFieldBorder
                    }
                }
            }
        }
    }

    function statusImage(type) {
        switch (type) {
        case PaymentChannelsListModel.ChannelType.Active:
            return "qrc:/images/activeChannel.png"
        case PaymentChannelsListModel.ChannelType.Pending:
            return "qrc:/images/pendingChannel.png"
        case PaymentChannelsListModel.ChannelType.Inactive:
            return "qrc:/images/inactiveChannel.png"
        case PaymentChannelsListModel.ChannelType.Closing:
            return "qrc:/images/inactiveChannel.png"
        case PaymentChannelsListModel.ChannelType.ForceClosing:
            return "qrc:/images/closingChannel.png"
        }

        return ""
    }

    function statusText(type) {
        switch (type) {
        case PaymentChannelsListModel.ChannelType.Active:
            return "Active"
        case PaymentChannelsListModel.ChannelType.Pending:
            return "Pending"
        case PaymentChannelsListModel.ChannelType.Inactive:
            return "Inactive"
        case PaymentChannelsListModel.ChannelType.Closing:
            return "Closing"
        case PaymentChannelsListModel.ChannelType.ForceClosing:
            return "Force closing"
        }

        return ""
    }

    function statusColor(type) {
        switch (type) {
        case PaymentChannelsListModel.ChannelType.Active:
            return "#3FBE4D"
        case PaymentChannelsListModel.ChannelType.Pending:
            return "#F09F1E"
        case PaymentChannelsListModel.ChannelType.Inactive:
            return "grey"
        case PaymentChannelsListModel.ChannelType.Closing:
            return "#E2344F"
        case PaymentChannelsListModel.ChannelType.ForceClosing:
            return "#E2344F"
        }

        return "transparent"
    }

    function allowExtendChannel(expiresAtTime) {
        if (isNaN(expiresAtTime) || !channelsModel.canExtendRentalChannel(
                    expiresAtTime)) {
            showBubblePopup(
                        "Can't extend channel when remaining time is less than 10 minute")
            return false
        }

        if (!checkPaymentNodeState(paymentNodeViewModel.stateModel)) {
            showBubblePopup("Can't extend channel until L2 is synced")
            return false
        }
        return true
    }
}
