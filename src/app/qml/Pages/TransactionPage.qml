import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import "../Views"
import "../Popups"

import com.xsn.viewmodels 1.0

Page {
    id: root
    background: Rectangle {
        color: "transparent"
    }


    TransactionsListView {
        anchors.fill: parent
        anchors.margins: 40
        transactionListModel: ApplicationViewModel.allTransactionsListModel
    }
}
