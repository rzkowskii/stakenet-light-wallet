import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12

import "../Components"

import com.xsn.utils 1.0

TextField {
    FontLoader { id: regularFont; source: "qrc:/Rubik-Regular.ttf" }
    font.family: regularFont.name
    font.pixelSize: 12
    color: SkinColors.mainText
    placeholderTextColor: SkinColors.secondaryText
    horizontalAlignment: TextInput.AlignRight

    background: Rectangle {
        color: SkinColors.mainBackground
        radius: 15
        border.width: 1
        border.color: activeFocus ? SkinColors.menuBackgroundGradienRightLine : SkinColors.popupFieldBorder
    }
}
