import QtQuick 2.6
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.0
import "../Components"

import com.xsn.models 1.0
import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

ColumnLayout {
    property WalletDexViewModel walletDexViewModel
    spacing: 0

    FontLoader {
        id: fontRegular
        source: "qrc:/Rubik-Regular.ttf"
    }
    FontLoader {
        id: fontMedium
        source: "qrc:/Rubik-Medium.ttf"
    }


    OrderHistoryHeaderView {
        Layout.fillWidth: true
        Layout.preferredHeight: 40
    }

    ListView {
        id: listView
        Layout.fillHeight: true
        Layout.fillWidth: true
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        spacing: 3

        model: QMLSortFilterListProxyModel {
            source: QMLSortFilterListProxyModel {
                source: walletDexViewModel.stateModel.ownOrdersHistoryListModel
                filterRole: "pairId"
                filterString: "%1/%2".arg(currentBaseAssetSymbol).arg(
                                  currentQuoteAssetSymbol)
            }
            sortRole: "time"
            sortCaseSensitivity: Qt.CaseInsensitive
            sortAsc: false
        }

        ScrollBar.vertical: ScrollBar {
            size: parent.height / parent.contentHeight
        }

        delegate: DelegateBackgroundItem {
            height: 35
            width: listView.width

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 15
                anchors.rightMargin: 15
                spacing: 0

                OrderBookText {
                    Layout.preferredWidth: 0.2 * parent.width
                    horizontalAlignment: Text.AlignHCenter
                    text: model.time
                }

                OrderBookText {
                    Layout.preferredWidth: 0.1 * parent.width
                    text: model.pairId
                    horizontalAlignment: Text.AlignHCenter
                    visible: false
                }

                OrderBookText {
                    Layout.preferredWidth: 0.1 * parent.width
                    horizontalAlignment: Text.AlignHCenter
                    text: model.type === OwnOrdersHistoryListModel.Type.Deal ? model.isBuy ? "Buy" : "Sell" : "order fee"
                    color: model.isBuy ? "green" : "red"
                }

                OrderBookText {
                    Layout.preferredWidth: 0.3 * parent.width
                    horizontalAlignment: Text.AlignHCenter
                    text: Utils.formatBalance(model.price)
                }

                OrderBookText {
                    Layout.preferredWidth: 0.2 * parent.width
                    horizontalAlignment: Text.AlignHCenter
                    text: Utils.formatBalance(model.amount)
                }

                OrderBookText {
                    Layout.preferredWidth: 0.2 * parent.width
                    horizontalAlignment: Text.AlignHCenter
                    text: Utils.formatBalance(model.total)
                }
            }
        }
    }
}
