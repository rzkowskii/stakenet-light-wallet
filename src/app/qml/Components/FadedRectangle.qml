import QtQuick 2.6
import QtQuick.Controls 2.2

import "../Components"

import com.xsn.utils 1.0

Rectangle {
    color: inactiveStateColor
    border.color: inactiveBorderColor
    property string activeStateColor: ""
    property string inactiveStateColor: ""
    property string activeBorderColor: ""
    property string inactiveBorderColor: "transparent"
    
    AnimationColor on color {
        id: animationBgActive
        propAnimation.to: activeStateColor;
    }
    
    AnimationColor on color {
        id: animationBgInactive
        propAnimation.to: inactiveStateColor
    }

    SequentialAnimation on border.color {
        id: animationBorderActive
        running: false

        PropertyAnimation {
            to: activeBorderColor
        }
    }

    SequentialAnimation on border.color {
        id: animationBorderInactive
        running: false

        PropertyAnimation {
            to: inactiveBorderColor
        }
    }

    function startFade() {
        animationBgInactive.stop();
        animationBgActive.start();
    }

    function stopFade() {
        animationBgActive.stop();
        animationBgInactive.start();
    }

    function borderStartFade() {
        animationBorderInactive.stop();
        animationBorderActive.start();
    }

    function borderStopFade() {
        animationBorderActive.stop();
        animationBorderInactive.start();
    }
}
