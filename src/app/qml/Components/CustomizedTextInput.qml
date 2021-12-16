import QtQuick 2.12
import QtQuick.Controls 2.2

import com.xsn.utils 1.0

TextInput {
    id: textInput

    font.family: regularFont.name
    font.pixelSize: 14
    color: SkinColors.mainText

    property string placeholderText: ""

    Label {
        text: textInput.placeholderText
        font: textInput.font
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        visible: textInput.text.length === 0
        color: SkinColors.comboboxIndicatorColor
    }
}
