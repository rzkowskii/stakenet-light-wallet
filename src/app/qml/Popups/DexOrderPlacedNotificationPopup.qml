import QtQuick 2.6
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import "../Components"

import com.xsn.viewmodels 1.0
import com.xsn.models 1.0
import com.xsn.utils 1.0

ActionDialog {
    id: root
    width: 440
    height: 180

    property string errorMessage: ""
    FontLoader { id: regularFont; source: "qrc:/Rubik-Regular.ttf" }


    ColumnLayout {
        anchors.fill: parent
        spacing: 40

        CloseButton {
            Layout.preferredHeight: 30
            Layout.preferredWidth: 30
            Layout.alignment: Qt.AlignRight | Qt.AlignTop
            onClicked: root.close()
        }

        Text {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignCenter
            font.family: regularFont.name
            font.pixelSize: 16

            text: errorMessage
            color: SkinColors.mainText
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
