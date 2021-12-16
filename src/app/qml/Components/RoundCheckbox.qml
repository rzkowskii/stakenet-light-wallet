import QtQuick 2.12
import QtQuick.Controls 2.2

import com.xsn.utils 1.0

CheckBox {
    id: control
    checked: false  

    indicator: Rectangle {
        implicitWidth: 26
        implicitHeight: 26
        x: control.leftPadding
        y: parent.height / 2 - height / 2
        radius: 10
        border.color: SkinColors.assetsRoundedCheckboxBorder
        color: "transparent"


        ColorOverlayImage {
            width: 14
            height: 14
            x: 5
            y: 6
            imageSize: 23
            color: SkinColors.mainText
            imageSource: "qrc:/images/checkmark-round.png"
            visible: control.checked
        }
    }
}
