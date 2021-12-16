#ifndef MOUSEEVENTSPY_HPP
#define MOUSEEVENTSPY_HPP

#include <QObject>
#include <QPoint>

class MouseEventSpy : public QObject {
    Q_OBJECT
public:
    explicit MouseEventSpy(QObject* parent = nullptr);
    static MouseEventSpy* Instance();

protected:
    bool eventFilter(QObject* watched, QEvent* event);

signals:
    void mouseEventDetected();

public slots:
    void setEventFilerEnabled(bool value);

private:
    bool _enabled = true;
};

#endif // MOUSEEVENTSPY_HPP
