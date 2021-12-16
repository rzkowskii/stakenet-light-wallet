#ifndef AUTOMATICHTLCRESOLVER_HPP
#define AUTOMATICHTLCRESOLVER_HPP

#include <QObject>

namespace swaps {

class AutoHTLCResolver : public QObject {
    Q_OBJECT
public:
    explicit AutoHTLCResolver(QObject* parent = nullptr);

signals:
};
}

#endif // AUTOMATICHTLCRESOLVER_HPP
