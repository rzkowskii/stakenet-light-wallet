import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.0

import com.xsn.utils 1.0

Text {
    FontLoader { id: localFont; source: "qrc:/Rubik-Light.ttf" }
    font.family: localFont.name
    font.pixelSize: 16
    color: SkinColors.mainText
}
