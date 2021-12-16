import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0

import "../Views"
import "../Components"
import "../Popups"

import com.xsn.viewmodels 1.0
import com.xsn.models 1.0
import com.xsn.utils 1.0

Rectangle {
    id: searchButton
    anchors.bottomMargin: 5
    border.width: 1
    border.color: SkinColors.menuBackgroundGradientFirst
    color:  SkinColors.mobileButtonBackground
    radius: 18
    width: 56
    height: 36
    z: 1000
    state: 'hidden'

    property alias filterString: searchArea.text

    states: [
        State {
            name: "hidden"
            PropertyChanges {
                target: searchButton
            }
        },
        State {
            name: "visible"
            PropertyChanges {
                target: searchButton
            }
        }
    ]

    transitions: [
        Transition {
            from: "hidden"
            to: "visible"

            PropertyAnimation {
                target: searchButton
                property: "width"
                to: 200
            }
            PropertyAnimation {
                target: searchArea
                property: "visible"
                to: true
            }
        },
        Transition {
            from: "visible"
            to: "hidden"

            NumberAnimation {
                target: searchButton
                property: "width"
                to: 56
                duration: 400
            }
        }
    ]

    RowLayout {
        anchors.fill: parent
        spacing: 5

        TextField {
            id: searchArea
            Layout.fillWidth: true
            Layout.leftMargin: 10

            visible: false
            font.family: fontRegular.name
            background: Rectangle {
                color: "transparent"
            }
            placeholderText: "Search wallet..."
            placeholderTextColor: SkinColors.mainText
            color: SkinColors.mainText
        }

        ColorOverlayImage {
            Layout.alignment: Qt.AlignRight
            imageSize: 30
            Layout.preferredWidth: imageSize
            Layout.rightMargin: 13
            height: imageSize
            imageSource: "qrc:/images/magnifyingGlass.png"
            color: SkinColors.magnifyingGlass

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    if(searchButton.state === 'visible')
                    {
                        searchArea.text = ""
                        searchButton.state = 'hidden'
                    }
                    else
                    {
                        searchButton.state = 'visible'
                    }
                }
            }
        }
    }
}
