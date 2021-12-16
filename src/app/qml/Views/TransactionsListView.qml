import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import Qt.labs.settings 1.0
import QtGraphicalEffects 1.0

import com.xsn.models 1.0
import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

import "../Components"

Item {
    id: root
    property QtObject transactionListModel: undefined
    property int confirmationsForApproved: 0
    signal openInExplorerRequested(string txid)
    signal lnInvoicePayReqRequested(string rHash)

    ColumnLayout {
        anchors.fill: parent

        RowLayout {
            Layout.fillWidth: true
            Text {
                Layout.alignment: Qt.AlignLeft
                text: qsTr("Transactions")
                font.pixelSize: 14
                color: SkinColors.mainText
            }

            Item {
                Layout.fillWidth: true
            }

            Item {
                Layout.preferredWidth: 250
                Layout.preferredHeight: 35

                Rectangle {
                    id: backGroundRect
                    anchors.fill: parent
                    radius: 10
                    color: SkinColors.headerBackground
                }

                RowLayout {
                    anchors.fill: parent
                    spacing: 5

                    SearchTextField {
                        id: searchField
                        Layout.leftMargin: 7
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        placeholderText: "Search Transactions"
                        validator: RegExpValidator {
                            regExp: /[a-zA-Z]{1,10}\D/g
                        }
                    }

                    ColorOverlayImage {
                        Layout.alignment: Qt.AlignRight
                        Layout.rightMargin: 7
                        imageSize: 32
                        width: imageSize
                        height: imageSize
                        opacity: 0.5
                        imageSource: "qrc:/images/magnifyingGlass.png"
                        color: SkinColors.magnifyingGlass
                    }
                }
            }
        }

        TransactionsListHeaderView {
            id: transactionsHeaderView
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            tabsModel: ListModel {
                ListElement {
                    role: "activity"
                    name: "ACTIVITY"
                    size: 0.28
                }
                ListElement {
                    role: "txDate"
                    name: "DATE"
                    size: 0.28
                }
                ListElement {
                    role: "status"
                    name: "STATUS"
                    size: 0.17
                }
                ListElement {
                    role: "txAmount"
                    name: "BALANCE CHANGE"
                    size: 0.27
                }
            }
        }

        XSNLabel {
            visible: transactionsList.count === 0
            text: "No Transactions"
            color: SkinColors.mainText
            opacity: 0.2
            font.pixelSize: 22
        }

        ListView {
            id: transactionsList

            Layout.fillHeight: true
            Layout.fillWidth: true

            boundsBehavior: Flickable.StopAtBounds

            model: QMLSortFilterListProxyModel {
                id: filterModel
                source: QMLSortFilterListProxyModel {
                    id: isDexFilterModel
                    source: transactionListModel
                    filterRole: "isDexTx"
                    filterString: transactionsFilterString()
                    filterCaseSensitivity: Qt.CaseInsensitive
                }
                sortRole: transactionsHeaderView.currentSortRole
                sortAsc: transactionsHeaderView.orderAsc
                filterCaseSensitivity: Qt.CaseInsensitive
                filterString: searchField.text
            }

            clip: true

            delegate: contactDelegate

            focus: true
            spacing: 0

            add: Transition {
                NumberAnimation {
                    properties: "y"
                    from: transactionsList.height
                    duration: 200
                }
            }
            addDisplaced: Transition {
                NumberAnimation {
                    properties: "x,y"
                    duration: 200
                }
            }
        }

        Component {
            id: contactDelegate

            DelegateBackgroundItem {
                id: item
                property bool isCurrentItem: ListView.isCurrentItem
                width: transactionsList.width
                height: 40

                onSelected: {
                    if (transactionsList.currentIndex === index) {
                        transactionsList.currentIndex = -1
                    } else {
                        transactionsList.currentIndex = index
                    }

                    if (model.txType === WalletTransactionsListModel.TransactionType.OnChain) {
                        var onChainTxDialog = openTransactionDetailsDialog({
                                                                               "assetID": currentAssetID,
                                                                               "symbol": model.symbol,
                                                                               "transaction": Qt.binding(function () {
                                                                                   return model.txData
                                                                               })
                                                                           })

                        onChainTxDialog.openUrlRequested.connect(
                                    openInExplorerRequested)
                    } else if (model.txType
                               === WalletTransactionsListModel.TransactionType.LightningInvoice) {
                        var lnInvoiceDialog = openLnInvoiceDetailsDialog({
                                                                             "assetID": currentAssetID,
                                                                             "symbol": model.symbol,
                                                                             "transaction": Qt.binding(function () {
                                                                                 return model.txData
                                                                             })
                                                                         })

                        lnInvoiceDialog.lnInvoiceRequested.connect(
                                    lnInvoicePayReqRequested)
                    } else if (model.txType
                               === WalletTransactionsListModel.TransactionType.LightningPayment) {
                        var lnPaymentDialog = openLnPaymentDetailsDialog({
                                                                             "assetID": currentAssetID,
                                                                             "symbol": model.symbol,
                                                                             "transaction": Qt.binding(function () {
                                                                                 return model.txData
                                                                             })
                                                                         })
                    } else if (model.txType
                               === WalletTransactionsListModel.TransactionType.EthOnChainTx) {
                        var ethChainTxDialog = openEthTransactionDetailsDialog({
                                                                                   "assetID": currentAssetID,
                                                                                   "symbol": model.symbol,
                                                                                   "transaction": Qt.binding(function () {
                                                                                       return model.txData
                                                                                   })
                                                                               })

                        ethChainTxDialog.openUrlRequested.connect(
                                    openInExplorerRequested)
                    } else if(model.txType === WalletTransactionsListModel.TransactionType.ConnextPayment){
                        var connextPaymentDialog = openConnextPaymentDetailsDialog({
                                                                             "assetID": currentAssetID,
                                                                             "symbol": model.symbol,
                                                                             "transaction": Qt.binding(function () {
                                                                                 return model.txData
                                                                             })
                                                                         })
                    }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 15
                    anchors.rightMargin: 15
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 0

                    Row {
                        Layout.preferredWidth: parent.width * 0.28
                        spacing: 15

                        Rectangle {
                            anchors.verticalCenter: parent.verticalCenter
                            width: 2
                            height: 16
                            color: txData.txAmount > 0 ? "#57ff5b" : SkinColors.walletAssetHighlightColor
                        }

                        XSNLabel {
                            anchors.verticalCenter: parent.verticalCenter
                            text: model.activity
                            color: "#bababa"
                            font.pixelSize: 11
                            font.capitalization: Font.Capitalize
                            font.family: fontRegular.name
                        }
                    }

                    XSNLabel {
                        Layout.preferredWidth: parent.width * 0.28
                        text: Utils.formatDate(txDate)
                        color: "#bababa"
                        font.pixelSize: 11
                        font.family: fontRegular.name
                    }

                    XSNLabel {
                        Layout.preferredWidth: parent.width * 0.17
                        text: model !== null ? model.status : ""
                        color: model !== null ? txData.statusColor : SkinColors.mainText
                        font.capitalization: Font.AllUppercase
                        font.pixelSize: 11
                        font.family: fontRegular.name
                    }

                    RowLayout {
                        Layout.preferredWidth: parent.width * 0.27
                        Layout.alignment: Qt.AlignLeft
                        spacing: 3

                        XSNLabel {
                            Layout.alignment: Qt.AlignVCenter
                            text: model.txAmount > 0 ? "+" : "-"
                            font.pixelSize: 11
                            font.family: fontRegular.name
                            color: "#bababa"
                        }

                        XSNLabel {
                            Layout.alignment: Qt.AlignVCenter
                            text: delta == "0.0" ? "< 0.00000001" : delta
                            color: "#bababa"
                            font.pixelSize: 11
                            font.family: fontRegular.name
                        }

                        XSNLabel {
                            Layout.alignment: Qt.AlignVCenter
                            text: symbol
                            color: "#bababa"
                            font.pixelSize: 11
                            font.capitalization: Font.AllUppercase
                            font.family: fontRegular.name
                        }

                        Item {
                            Layout.fillWidth: true
                        }

                        XSNLabel {
                            Layout.alignment: Qt.AlignVCenter
                            text: ". . ."
                            rotation: 90
                            font.pixelSize: 13
                            color: SkinColors.mainText
                        }
                    }
                }
            }
        }
    }

    Settings {
        id: settings
        category: "Dex"
    }

    function transactionsFilterString() {

        var txFilterString = settings.value("showTxType")

        switch (txFilterString) {
        case "All":
            return ""
        case "Hide Dex transactions":
            return false
        }
    }

    Connections {
        target: menuStackLayout

        function onCurrentIndexChanged() {
            isDexFilterModel.filterString = transactionsFilterString()
        }
    }
}
