import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0

import "../Components"

import com.xsn.models 1.0
import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

ColumnLayout {
    id: root
    property WalletDexViewModel walletDexViewModel
    signal selectTrade(double amount, double price, bool isBuy)
    spacing: 0

    RoundedRectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 45
        corners.topLeftRadius: 10
        corners.topRightRadius: 10
        customGradient: {
            "vertical": true,
            "colors": [{
                           "position": 0.0,
                           "color": SkinColors.delegatesBackgroundLightColor
                       }, {
                           "position": 1.0,
                           "color": SkinColors.delegatesBackgroundDarkColor
                       }]
        }

        RowLayout {
            anchors.fill: parent
            anchors.rightMargin: 15
            anchors.leftMargin: 15
            spacing: 0

            Repeater {
                model: ListModel {
                    ListElement {name: "Amount"; size: 0.25}
                    ListElement {name: "Time"; size: 0.38}
                    ListElement {name: "Price"; size: 0.37}
                }

                delegate:  OrderBookHeaderText {
                    Layout.preferredWidth: parent.width * model.size
                    Layout.fillHeight: true
                    text: walletDexViewModel && walletDexViewModel.hasSwapPair && model.name !== "Time" ? "%1 (%2)".arg(model.name) .arg(model.name === "Amount" ? currentBaseAssetSymbol: currentQuoteAssetSymbol) : model.name
                    font.capitalization: Font.AllUppercase
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }


    ListView {
        id: listView
        Layout.fillHeight: true
        Layout.fillWidth: true
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        spacing: 3

        model: QMLSortFilterListProxyModel {
            source: walletDexViewModel.stateModel.tradeHistoryListModel
            sortRole: "time"
            sortCaseSensitivity: Qt.CaseInsensitive
            sortAsc: false
        }

        ScrollBar.vertical: ScrollBar {
            size: parent.height / parent.contentHeight
        }

        delegate: DelegateBackgroundItem {
            id: item
            width: listView.width
            height: 32

            onSelected: root.selectTrade(model.amount, model.price, !model.isSold)

            RowLayout {
                anchors.fill: parent
                anchors.rightMargin: 15
                anchors.leftMargin: 15
                spacing: 0

                OrderBookText {
                    Layout.preferredWidth: parent.width * 0.25
                    Layout.fillHeight: true
                    text: Utils.formatBalance(model.amount)
                    horizontalAlignment: Text.AlignHCenter
                }

                OrderBookText {
                    Layout.fillHeight: true
                    Layout.preferredWidth: parent.width * 0.38
                    text: Utils.formatDate(model.time)
                    horizontalAlignment: Text.AlignHCenter
                }

                OrderBookText {
                    Layout.fillHeight: true
                    Layout.preferredWidth: parent.width * 0.37
                    text: "%1 %2".arg(Utils.formatBalance(model.price)).arg(
                              currentQuoteAssetSymbol)
                    color: model.isSold ? SkinColors.transactionItemSent : "#3FBE4D"
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }
    }
}
