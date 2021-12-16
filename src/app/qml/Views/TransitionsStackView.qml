import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import "../Components"


StackView {
    id: stackView
    pushEnter: Transition {
        id: pushEnter
        ParallelAnimation {
            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 500; easing.type: Easing.OutCubic }
        }
    }

    pushExit: Transition {
    }
    
    popExit: Transition {
        id: popExit
        ParallelAnimation {
            NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 500; easing.type: Easing.OutCubic }
        }
    }

    popEnter: Transition {
    }
}
