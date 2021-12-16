import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import "../Views"
import "../Components"
import "../Popups"

import com.xsn.viewmodels 1.0
import com.xsn.models 1.0
import com.xsn.utils 1.0

Page {
    id: root
    property string accountBalance: portofiloWalletsListView.accountBalance
    property alias rootLayout: layout

    signal balanceVisibileChanged

    onBalanceVisibileChanged: {
        portofiloWalletsListView.balanceVisibilityChanged()
    }

    background: Rectangle {
        anchors.fill: parent
        color: "transparent"
    }

    ColumnLayout {
        id: layout
        anchors.fill: parent
        anchors.rightMargin: isMobile ? 15 : 30
        anchors.leftMargin: isMobile ? 15 : 30
        anchors.topMargin: isMobile ? 50 : 30
        anchors.bottomMargin: isMobile ? 10 : 30
        spacing: 35

        PageHeaderView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            accountBalance: portofiloWalletsListView.accountBalance
            isApproximatelyEqual: portofiloWalletsListView.isApproximatelyEqual
        }

        PortofiloWalletsListView {
            id: portofiloWalletsListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: !isMobile
            onSendCoins: {
                openSendDialog(id);
            }
            onReceiveCoins: {
                openReceiveDialog(id);
            }
        }
    }
}
