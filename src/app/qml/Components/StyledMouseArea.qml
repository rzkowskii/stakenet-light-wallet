import QtQuick 2.6

MouseArea {
    acceptedButtons: Qt.RightButton
    hoverEnabled: true
    cursorShape: Qt.IBeamCursor
    
    property int selectStart;
    property int selectEnd;
    property int curPos;
}
