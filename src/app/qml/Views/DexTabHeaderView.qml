import QtQuick 2.0
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0
import "../Components"

import com.xsn.utils 1.0

Item {

    Rectangle {
        anchors.fill: parent
        color: SkinColors.menuBackground
        opacity: 0.81
    }

    RowLayout {
        anchors.fill: parent
        spacing: 50

//        TabDelegate {
//            Layout.alignment: Qt.AlignBottom
//            Layout.preferredWidth: 210
//            Layout.preferredHeight: 60
//            isCurrentTab: true
//        }
    }
}
