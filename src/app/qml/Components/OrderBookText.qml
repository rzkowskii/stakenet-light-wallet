import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import com.xsn.utils 1.0

Text {
    FontLoader { id: fontLight; source: "qrc:/Rubik-Light.ttf" }
    font.family: fontLight.name
    color: SkinColors.mainText
    font.pixelSize: 10
    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignRight
    font.capitalization: Font.AllUppercase
}
