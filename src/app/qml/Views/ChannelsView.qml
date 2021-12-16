import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0

import com.xsn.viewmodels 1.0
import com.xsn.models 1.0
import com.xsn.utils 1.0

import "../Components"
import "../Views"
import "../Popups"

Item {
    id: root
    property int assetID
    property PaymentNodeViewModel payNodeViewModel
    property string currentSymbol: ""
    property string currentAssetName: ""
    property int confirmationsForChannelApproved: 0
    property PaymentChannelsListModel channelsModel: payNodeViewModel.channelsModel
    property bool isActiveLightning: ApplicationViewModel.paymentNodeViewModel.paymentNodeActivity(
                                         assetID)

    WalletAssetsListModel {
        id: assetModel
        Component.onCompleted: {
            initialize(ApplicationViewModel)
        }
    }

    Connections {
        target: payNodeViewModel
        function onCurrentAssetIDChanged() {
            root.isActiveLightning = ApplicationViewModel.paymentNodeViewModel.paymentNodeActivity(
                        assetID)
        }
    }

    WalletDexChannelBalanceViewModel {
        id: channelBalancesViewModel

        Component.onCompleted: {
            initialize(ApplicationViewModel)
        }
    }

    OpenChannelViewModel {
        id: openChannelViewModel
        currentAssetID: assetID
        Component.onCompleted: {
            initialize(ApplicationViewModel)
        }
        onChannelOpened: {
           channelsModel.refresh()
        }
    }

    Connections {
        target: payNodeViewModel

        function onChannelClosed() {
            channelsModel.refresh()
        }

        function onAllChannelsClosed() {
            channelsModel.refresh()
        }

        function onRestoreChannelFinished() {
            showBubblePopup("Channel was successfully restored!")
        }

        function onRestoreChannelFailed(errMsg) {
            showBubblePopup("Failed to restore channel: %1!".arg(errMsg))
        }

        function onReconcileFinished() {
            showBubblePopup("Channel was successfully reconciled!")
        }

        function onReconcileFailed(errMsg) {
            showBubblePopup("Failed to reconcile channel: %1!".arg(errMsg))
        }
    }

    Connections {
        target: ApplicationViewModel.paymentNodeViewModel

        function onNoFreeSlots(lnds) {
            var popup = openDialog(chooseLightningComponent, {
                                       "lightningsList": lnds,
                                       "assetModel": assetModel,
                                       "assetID": currentAssetID,
                                       "assetName": currentAssetName
                                   })
        }

        function onNodeActivityChanged() {
            root.isActiveLightning = ApplicationViewModel.paymentNodeViewModel.paymentNodeActivity(
                        assetID)
        }
    }

    Component {
        id: closeChannelDialogComponent
        CloseChannelPopup {}
    }

    Component {
        id: lnBackupsDialogComponent
        LightningBackupPopup {}
    }

    Component {
        id: channelDetailsComponent
        ChannelDetailsPopup {}
    }

    Component {
        id: connextChannelDetailsComponent
        ConnextChannelDetailsPopup {}
    }

    Component {
        id: extenderRentalComponent
        ExtenderRentalPopup {}
    }

    Component {
        id: depositDialog
        DepositChannelPopup {}
    }

    Component {
        id: withdrawDialog
        WithdrawPopup {}
    }

    Component {
        id: rentalNotificationComponent

        ConfirmationPopup {
            width: 480
            height: 280
            cancelButton.visible: false
            confirmButton.text: "Ok"
            onConfirmClicked: {
                close()
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        Loader {
            Layout.maximumHeight: 40
            Layout.fillWidth: true
            asynchronous: true
            visible: !isActiveLightning /*status == Loader.Ready*/
            source: "qrc:/Views/ChannelsHeaderView.qml" // TODO: header for Connext
            property PaymentNodeViewModel paymentNodeViewModel: payNodeViewModel
            property WalletAssetsListModel walletAssetModel: assetModel
            property bool isActiveLightning: root.isActiveLightning
            property int assetID: root.assetID
            property string symbol: currentSymbol
            property string assetName: currentAssetName
        }

        RowLayout {
            Layout.fillWidth: true
            visible: isActiveLightning

            Text {
                Layout.alignment: Qt.AlignLeft
                text: qsTr("Channels")
                font.pixelSize: 14
                color: SkinColors.mainText
            }

            Item {
                Layout.fillWidth: true
            }

            Row {
                Layout.preferredWidth: 120
                spacing: 4
                visible: payNodeViewModel.stateModel.identifier.length !== 0

                XSNLabel {
                    font.pixelSize: 12
                    color: SkinColors.menuItemText
                    text: "Public key: "
                    font.family: regularFont.name
                }

                Row {
                    spacing: 5

                    XSNLabel {
                        id: pubKeyAddress
                        font.pixelSize: 12
                        text: payNodeViewModel.stateModel.identifier
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
                Layout.fillWidth: true
            }

            RowLayout {
                spacing: 20

                Repeater {
                    model: ListModel {
                        ListElement {
                            text: "New Channel+"
                            action: function () {
                                if (allowOpenChannel())
                                    var popup = openChannelDialog({
                                                                      "assetID": assetID
                                                                  })
                            }
                        }

                        ListElement {
                            text: "Channel Rental"
                            action: function () {
                                if (allowOpenRentalChannel())
                                    var popup = openChannelRentalDialog({
                                                                            "assetID": assetID,
                                                                            "symbol": currentSymbol
                                                                        })
                            }
                        }

                        ListElement {
                            text: "Backup"
                            action: function () {

                                if (payNodeViewModel.type === Enums.PaymentNodeType.Lnd) {

                                    openDialog(lnBackupsDialogComponent, {
                                                   "assetID": assetID,
                                                   "currentName": currentAssetName,
                                                   "currentSymbol": currentSymbol
                                               })
                                } else {
                                    payNodeViewModel.restoreChannel()
                                }
                            }
                        }
                    }

                    Text {
                        Layout.alignment: Qt.AlignVCenter
                        text: model.text === "Backup" &&
                              payNodeViewModel.type === Enums.PaymentNodeType.Connext ?
                                  "Restore channel" : model.text
                        font.pixelSize: 11
                        font.bold: true
                        color: SkinColors.mainText
                        opacity: 0.5

                        PointingCursorMouseArea {
                            onClicked: action()
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true

            visible: channelsListView.count > 0
            spacing: 0

            RoundedRectangle {
                Layout.fillHeight: true
                Layout.preferredWidth: parent.width * 0.33
                corners.topLeftRadius: 10

                customGradient: {
                    "vertical": false,
                    "colors": [{
                                   "position": 0.0,
                                   "color": '#122145'
                               }, {
                                   "position": 1.0,
                                   "color": SkinColors.delegatesBackgroundLightColor
                               }]
                }

                ColumnLayout {
                    anchors {
                        fill: parent
                        topMargin: 20
                        bottomMargin: 20
                        leftMargin: 20
                        rightMargin: 20
                    }
                    spacing: 20

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.rightMargin: 20
                        Layout.leftMargin: 5

                        spacing: 15

                        Image {
                            Layout.alignment: Qt.AlignTop
                            source: "qrc:/images/icon-24-lighting.svg"
                            sourceSize: Qt.size(15, 15)
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Text {
                                Layout.fillWidth: true
                                horizontalAlignment: Text.AlignLeft
                                text: qsTr("CAN SEND")
                                font.pixelSize: 11
                                color: SkinColors.mainText
                            }

                            Text {
                                Layout.fillWidth: true
                                horizontalAlignment: Text.AlignLeft
                                text: Utils.formatBalance(
                                          payNodeViewModel.stateModel.nodeLocalRemoteBalances.allLocalBalance)
                                font.pixelSize: 17
                                color: SkinColors.mainText
                            }
                        }

                        Image {
                            sourceSize: Qt.size(25, 44)
                            source: "qrc:/images/icon-can-send.svg"
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 12
                        radius: 12
                        color: '#070c19'

                        ProgressBar {
                            id: channelBalanceBrogressBar
                            anchors.verticalCenter: parent.verticalCenter
                            width: parent.width
                            height: 4
                            opacity: 0.5
                            value: payNodeViewModel.stateModel.nodeLocalRemoteBalances.allLocalBalance / (payNodeViewModel.stateModel.nodeLocalRemoteBalances.allLocalBalance + payNodeViewModel.stateModel.nodeLocalRemoteBalances.allRemoteBalance)

                            background: Rectangle {
                                implicitWidth: channelBalanceBrogressBar.width
                                implicitHeight: channelBalanceBrogressBar.height
                                color: '#ff6868'
                                radius: 3
                            }

                            contentItem: Item {
                                implicitWidth: channelBalanceBrogressBar.width
                                implicitHeight: channelBalanceBrogressBar.height

                                Rectangle {
                                    width: channelBalanceBrogressBar.visualPosition * parent.width
                                    height: parent.height
                                    radius: 3
                                    color: "#57ff5b"
                                }
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.leftMargin: 20
                        Layout.rightMargin: 5

                        spacing: 15

                        Image {
                            sourceSize: Qt.size(25, 44)
                            source: "qrc:/images/icon-can-receive.svg"
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Text {
                                Layout.fillWidth: true
                                horizontalAlignment: Text.AlignRight
                                text: qsTr("CAN RECEIVE")
                                font.pixelSize: 11
                                color: SkinColors.mainText
                            }

                            Text {
                                Layout.fillWidth: true
                                horizontalAlignment: Text.AlignRight
                                text: Utils.formatBalance(
                                          payNodeViewModel.stateModel.nodeLocalRemoteBalances.allRemoteBalance)
                                font.pixelSize: 17
                                color: SkinColors.mainText
                            }
                        }

                        Image {
                            Layout.alignment: Qt.AlignTop
                            source: "qrc:/images/icon-24-lighting.svg"
                            sourceSize: Qt.size(15, 15)
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.fillHeight: true
                Layout.fillWidth: true
                spacing: 0

                TransactionsListHeaderView {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    Layout.preferredHeight: 40
                    corners.topLeftRadius: 0
                    property ListModel connextChannelsHeaderModel: ListModel {
                        ListElement {
                            role: "activity"
                            name: "CHANNEL"
                            size: 0.2
                        }

                        ListElement {
                            role: "activity"
                            name: "CAN SEND"
                            size: 0.2
                        }
                        ListElement {
                            role: "activity"
                            name: "CAN RECEIVE"
                            size: 0.22
                        }
                    }
                    property ListModel lndChannelsHeaderModel: ListModel {
                        ListElement {
                            role: "activity"
                            name: "CHANNEL"
                            size: 0.2
                        }
                        ListElement {
                            role: "activity"
                            name: "STATUS"
                            size: 0.15
                        }

                        ListElement {
                            role: "activity"
                            name: "CAN SEND"
                            size: 0.2
                        }

                        ListElement {
                            role: "activity"
                            name: "CAN RECEIVE"
                            size: 0.22
                        }

                        ListElement {
                            role: "activity"
                            name: "RENTAL TIME"
                            size: 0.33
                        }
                    }
                    currentIndex: payNodeViewModel.type === Enums.PaymentNodeType.Lnd ? 1 : 0
                    tabsModel: payNodeViewModel.type === Enums.PaymentNodeType.Lnd ? lndChannelsHeaderModel : connextChannelsHeaderModel
                }

                ChannelsListView {
                    id: channelsListView
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    model: channelsModel
                    paymentNodeViewModel: payNodeViewModel
                    currentAssetSymbol: currentSymbol
                    currentConfirmationsForApproved: confirmationsForChannelApproved
                    currentAssetID: root.assetID

                    onDeleteChannelRequested: {
                        if (allowCloseChannels()) {
                            var popup = openDialog(closeChannelDialogComponent,
                                                   {
                                                       "paymentNodeViewModel": payNodeViewModel,
                                                       "assetID": assetID,
                                                       "channelOutpoint": channelOutpoint,
                                                       "confirmMsg": delegate.type === PaymentChannelsListModel.ChannelType.Inactive ? "You are going to close inactive channel, your funds will return after %1 blocks. Close channel?".arg(channelCsvDelay) : delegate.type === PaymentChannelsListModel.ChannelType.Closing ? "You are going to close closing channel, your funds will return after %1 blocks. Close channel?".arg(channelCsvDelay) : "Are you sure you want to close the channel?",
                                                       "containsInactiveClosingChannel": delegate.type === PaymentChannelsListModel.ChannelType.Inactive || delegate.type === PaymentChannelsListModel.ChannelType.Closing
                                                   })
                        }
                    }

                    onOpenInExplorerRequested: {
                        openInExplorer(txid)
                    }
                }
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }

    function containsInactiveClosingChannel() {
        for (var index = 0; index < channelsModel.count; index++) {
            if (channelsModel.get(
                        index).type === PaymentChannelsListModel.ChannelType.Inactive
                    || channelsModel.get(
                        index).type === PaymentChannelsListModel.ChannelType.Closing) {
                return true
            }
        }
        return false
    }

    function allowOpenChannel() {
        if (!checkPaymentNodeState(payNodeViewModel.stateModel)) {
            showBubblePopup(
                        "Can't open channel until %1".arg(
                            payNodeViewModel.stateModel.nodeType
                            === Enums.PaymentNodeType.Lnd ? "L2 is synced" : "Connext is active"))
            return false
        }
        return true
    }

    function allowOpenRentalChannel() {
        if (allowOpenChannel()) {
            if (payNodeViewModel.stateModel.nodeType === Enums.PaymentNodeType.Connext
                    && channelsModel.rowCount() === 0) {
                showBubblePopup(
                            "The channel cannot be rented. Please open the channel first")
                return false
            }
            return true
        }
        return false
    }

    function allowDepositChannel() {
        if (!checkPaymentNodeState(payNodeViewModel.stateModel)) {
            showBubblePopup("Can't deposit channel until Connext is active")
            return false
        }
        return true
    }

    function allowReconcileChannel() {
        if (!checkPaymentNodeState(payNodeViewModel.stateModel)) {
            showBubblePopup("Can't reconcile channel until Connext is active")
            return false
        }
        return true
    }

    function allowWithdrawChannel() {
        if (!checkPaymentNodeState(payNodeViewModel.stateModel)) {
            showBubblePopup("Can't withdraw channel until Connext is active")
            return false
        }
        return true
    }

    function allowCloseChannels() {
        if (!checkPaymentNodeState(payNodeViewModel.stateModel)) {
            showBubblePopup("Can't close channels until L2 is synced")
            return false
        }
        return true
    }

    function allowOpenAPSettings() {
        if (payNodeViewModel.type === Enums.PaymentNodeType.Lnd
                && lnViewModel.stateModel.autopilotStatus
                !== LnDaemonStateModel.AutopilotStatus.AutopilotActive) {
            showBubblePopup(
                        "Can't open autopilot settings until autopilot is non active")
            return false
        }
        return true
    }
}
