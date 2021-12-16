import QtQuick 2.14

import com.xsn.utils 1.0

SequentialAnimation {
    running: false

    property alias propAnimation: propAnim
    
    ColorAnimation {
        id: propAnim
        duration: 350
    }
}
