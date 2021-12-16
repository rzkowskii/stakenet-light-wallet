import QtQuick 2.12
import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.0

import com.xsn.utils 1.0

ColumnLayout{

    XSNLabel {
        Layout.leftMargin: 30
        color: SkinColors.comboboxIndicatorColor
        font.pixelSize: 12
        text: qsTr("Send funds to")
    }

    Rectangle {

        Layout.leftMargin: 30
        color: SkinColors.mainBackground
        Layout.preferredWidth: 400
        Layout.preferredHeight: 30

        TextArea {
            Layout.leftMargin: 31
            font.bold: false;
            font.pixelSize: 14;
            color: SkinColors.popupInfoText
            placeholderText: "Enter the XSN address"

            cursorVisible: true
        }
    }
}
