import QtQuick 2.12
import QtQuick.Controls 2.12

import com.xsn.utils 1.0

RadioButton {
    id: control

    indicator: Rectangle {
        implicitWidth: 18
        implicitHeight: 18
        radius: 9
        border.color: control.checked ? SkinColors.menuBackgroundGradientFirst : SkinColors.popupFieldBorder
        color: "transparent"

        Rectangle {
            width: 12
            height: 12
            x: 3
            y: 3
            radius: 6
            color: control.checked ? SkinColors.menuBackgroundGradientFirst : SkinColors.popupFieldBorder
            visible: control.checked
        }
    }
}
