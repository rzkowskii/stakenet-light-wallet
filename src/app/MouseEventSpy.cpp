#include "MouseEventSpy.hpp"
#include <QEvent>
#include <QGuiApplication>
#include <QMouseEvent>

//==============================================================================

MouseEventSpy::MouseEventSpy(QObject* parent)
    : QObject(parent)
{
}

//==============================================================================

MouseEventSpy* MouseEventSpy::Instance()
{
    static MouseEventSpy instance;
    qGuiApp->installEventFilter(&instance);
    return &instance;
}

//==============================================================================

void MouseEventSpy::setEventFilerEnabled(bool value)
{
    if (_enabled != value) {
        _enabled = value;
    }
}

//==============================================================================

bool MouseEventSpy::eventFilter(QObject* watched, QEvent* event)
{
    if (_enabled) {
        QEvent::Type type = event->type();
        if ((type == QEvent::MouseButtonDblClick || type == QEvent::MouseButtonPress)
            && event->spontaneous()) {
            mouseEventDetected();
        }
    }
    return QObject::eventFilter(watched, event);
}

//==============================================================================
