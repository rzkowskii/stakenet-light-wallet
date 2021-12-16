import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

ColumnLayout {
    Layout.fillWidth: true
    Layout.leftMargin: 10
    spacing: 10

    FontLoader { id: regularFont; source: "qrc:/Rubik-Regular.ttf" }   
    FontLoader { id: lightFont; source: "qrc:/Rubik-Light.ttf" }

    Text {
        text: qsTr("Est. Assets Value")
        font.pixelSize: 12
        font.family: regularFont.name
        color: SkinColors.menuItemText
    }

    Text {
        font.family: lightFont.name
        text: qsTr("%1 %2") .arg(ApplicationViewModel.localCurrencyViewModel.currentCurrencySymbol) .arg(accountBalance)
        font.pixelSize: 24
        color: SkinColors.mainText
    }
}
