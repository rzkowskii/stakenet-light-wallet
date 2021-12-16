#ifndef QMLCLIPBOARDADAPTER_HPP
#define QMLCLIPBOARDADAPTER_HPP

#include <QObject>

class QClipboard;

class QMLClipboardAdapter : public QObject {
    Q_OBJECT
public:
    explicit QMLClipboardAdapter(QObject* parent = 0);

public slots:
    void setText(QString text);

private:
    QClipboard* _clipboard{ nullptr };
};

#endif // QMLCLIPBOARDADAPTER_HPP
