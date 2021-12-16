import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import "../Components"

import com.xsn.models 1.0
import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

Text {
    FontLoader { id: fontRegular; source: "qrc:/Rubik-Regular.ttf" }
    font.family: fontRegular.name
    font.pixelSize: 12
    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignRight
    color: SkinColors.secondaryText
    font.capitalization: Font.AllUppercase
}
