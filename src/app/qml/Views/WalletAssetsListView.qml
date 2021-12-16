import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0
import QtQuick.Controls 2.2
import QtQuick.Controls 2.12

import "../Components"

import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

ListView {
    id: listView

    property int actualIndex: 0
    property bool balanceVisible
    clip: true

    highlight: Item {
        z: 20
         RoundedRectangle {
            anchors.left: parent.left
            corners.topLeftRadius: 10
            corners.bottomLeftRadius: 10
            height: 70
            width: 5
            color: SkinColors.walletAssetHighlightColor
       }
    }

    highlightFollowsCurrentItem: true
    spacing: isMobile ? 8 : 6
    boundsBehavior: Flickable.StopAtBounds

    delegate: WalletDelegate {
        id: asset
        width: listView.width
        height: 70
        transactionListModel: walletViewModel.transactionsListModel

        confirmationsForApproved: model.confirmationsForApproved
        confirmationsForChannelApproved: model.confirmationsForChannelApproved
        assetColor: model.color
        assetID: model.id
        name: model.name
        symbol: model.symbol
        balance: model.balance
        balanceOnChain: model.balanceOnChain
        nodeBalance: model.nodeBalance
        averageSycBlockForSec: model.averageSycBlockForSec
        minLndCapacity: model.minLndCapacity
        chainID: model.chainId
        balanceVisible: listView.balanceVisible
        onAssetClicked: listView.currentIndex = index;
    }
}
