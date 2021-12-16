import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import "../Components"

import com.xsn.models 1.0
import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

ColumnLayout {
    property WalletDexViewModel walletDexViewModel
    property var popup
    spacing: 0

    Button {
        Layout.alignment: Qt.AlignRight
        checkable: true
        font.pixelSize: 10
        visible: openOrdersListView.count > 0

        contentItem: XSNLabel {
            text: "Cancel all orders"
            font.family: fontRegular.name
            font.pixelSize: 12
            color: "#E2344F"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            font.capitalization: Font.AllUppercase
        }

        background: Rectangle {
            implicitWidth: 130
            implicitHeight: 25
            color: SkinColors.orderBookEvenBackground
            border.width: 0.5
            border.color: SkinColors.popupFieldBorder
        }

        PointingCursorMouseArea {
            onClicked: {
                popup = openConfirmRemoveOrderDialog({ message: "Are you sure you want to cancel all orders?"})
                popup.confirmClicked.connect(cancelAllOrders);
            }
        }
    }

    OpenOrdersHeaderView {
        Layout.fillWidth: true
        Layout.preferredHeight: 40
    }

    OpenOrdersListView {
        id: openOrdersListView
        Layout.fillHeight: true
        Layout.fillWidth: true
        dexViewModel: walletDexViewModel
    }

    function cancelAllOrders()
    {
        walletDexViewModel.cancelAllOrders();
        popup.close();
    }
}
