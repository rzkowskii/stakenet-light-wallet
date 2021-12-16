import QtQuick 2.12

import com.xsn.utils 1.0

import "../Components"

Text {
    FontLoader {id: fontMedium; source: "qrc:/Rubik-Medium.ttf"}
    property bool isCurrentItem: false
    font.weight: isCurrentItem ? Font.Medium : Font.Normal
    font.pixelSize: 14
    font.family: fontMedium.name
    font.capitalization: Font.AllUppercase

    property string activeColor: SkinColors.mainText
    property string inactiveColor: SkinColors.headerText

    AnimationColor on color {
        id: animationActive
        propAnimation.to: activeColor
    }

    AnimationColor on color {
        id: animationInactive
        propAnimation.to: inactiveColor
    }

    function startFade() {
        animationInactive.stop()
        animationActive.start()
    }

    function stopFade() {
        animationActive.stop()
        animationInactive.start()
    }
}
