import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.15

import "../Views"
import "../Components"
import "../Popups"

import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0
import com.xsn.models 1.0

Item {
    property alias source: coinImage.source
    property alias color: glow.color
    property alias sourceSize: coinImage.sourceSize
    property alias radius: glow.radius
    property alias samples: glow.samples
    property alias spread: glow.spread

    Image {
        id: coinImage
    }
    
    Glow {
        id: glow
        anchors.fill: coinImage
        radius: 121
        spread: 0.1
        samples: 241
        source: coinImage
        opacity: 1.0
    }
}
