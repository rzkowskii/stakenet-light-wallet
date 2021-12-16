import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import "../Components"

import com.xsn.utils 1.0

ColumnLayout {
    spacing: 13
    
    WalletBusyIndicator {
        Layout.fillWidth: true
        color: SkinColors.secondaryText
    }

    Item {
        Layout.fillWidth: true
        Layout.preferredHeight: 15
        
        XSNLabel {
            anchors.centerIn: parent
            font.pixelSize: 14
            font.family: fontRegular.name
            text: "Please wait while the order book is being updated..."
            color: SkinColors.secondaryText
        }
    }  
}
