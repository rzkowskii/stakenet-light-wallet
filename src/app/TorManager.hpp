#ifndef TORMANAGER_HPP
#define TORMANAGER_HPP

#include <QNetworkReply>
#include <QObject>

class TorController;

class TorManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool torActiveState READ torActiveState NOTIFY torActiveStateChanged)
public:
    TorManager(QObject* parent = nullptr);
    ~TorManager();
    static TorManager* Instance();

    bool torActiveState() const;

signals:
    void torActiveStateChanged();

public slots:
    void changeTorState();

private:
    void writeSettings() const;
    void readSettings();
    void startTor();
    void stopTor();

private:
    bool _torActiveState = false;
};

#endif // TORMANAGER_HPP
