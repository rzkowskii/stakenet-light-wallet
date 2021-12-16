import QtQuick 2.12
import QtQuick.Layouts 1.3

import "../Components"
import com.xsn.utils 1.0


Rectangle {
    color: SkinColors.headerBackground
    radius: 4

    property string currentSortRole: layout.children[currentIndex].sortRole
    property bool orderAsc: false
    property int currentIndex: 1

    RowLayout {
        id: layout
        anchors.fill: parent
        anchors.leftMargin: 20
        anchors.rightMargin: 20
        anchors.topMargin: 10
        spacing: 10

        Repeater {
            model: ListModel {
                ListElement { role: "name"; name: "Currency"; size: 0.35}
                ListElement { role: "balance"; name: "Balance"; size: 0.25}
                ListElement { role: "percent"; name: "Portfolio %"; size: 0.2}
                ListElement { role: ""; name: ""; size: 0.15}
            }

            delegate: ListHeader {
                Layout.preferredWidth: parent.width * model.size
                Layout.alignment: Qt.AlignCenter
                headerText: model.name
                highlighted: currentIndex == index && model.role !== ""
                sortRole: model.role
                onListHeaderClicked: {
                    if(currentIndex == index) {
                        orderAsc = !orderAsc;
                    }
                    else {
                        currentIndex = index;
                        orderAsc = true;
                    }
                }
            }
        }
    }
}
