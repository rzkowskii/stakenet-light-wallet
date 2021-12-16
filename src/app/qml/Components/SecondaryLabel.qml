import QtQuick 2.9
import QtQuick.Controls 2.2

import com.xsn.utils 1.0

Label {
    FontLoader { id: mediumFont; source: "qrc:/Rubik-Medium.ttf" }
    font.family: mediumFont.name
    color: SkinColors.menuItemText
    font.pixelSize: 14
}
