#ifndef SUBCRIBEEVENTEMITTER_HPP
#define SUBCRIBEEVENTEMITTER_HPP

#include <QObject>

class SubcribeEventEmitter : public QObject
{
    Q_OBJECT
public:
    SubcribeEventEmitter(QObject *parent = nullptr);

signals:
   void disconnected();
   void eventReady();
};

#endif // SUBCRIBEEVENTEMITTER_HPP
