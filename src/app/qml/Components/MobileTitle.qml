import QtQuick 2.12

import com.xsn.utils 1.0

Text {
    FontLoader { id: fontRegular; source: "qrc:/Rubik-Regular.ttf" }
    font.family: fontRegular.name
    font.pixelSize: 14
    color: SkinColors.mobileTitle
    font.capitalization: Font.AllUppercase
}
