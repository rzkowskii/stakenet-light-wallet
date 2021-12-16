import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import "../Views"
import "../Components"

import com.xsn.viewmodels 1.0

Page {
    property string modelName: ""
    property var walletViewModel: ApplicationViewModel.emulator

    EmulatorView {
        id: emulatorView
        anchors.fill: parent
        onAddTransaction: walletViewModel.addTransaction(modelName, count);
        onClearTransactions: walletViewModel.clearTransactions(modelName);
        onGenerateBlocks: function(count, addressTo) {
            walletViewModel.requestNewBlock(currentAssetID, count, addressTo);
        }

        onReorgChain: function(disconnectCount, connectCount, addressTo) {
            walletViewModel.reorgRequested(currentAssetID, disconnectCount, connectCount, addressTo)
        }

        //maxHeight: walletViewModel.maxHeight
    }
}
