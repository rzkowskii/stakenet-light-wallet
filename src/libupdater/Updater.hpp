#ifndef UPDATER_HPP
#define UPDATER_HPP

#include <QJsonObject>
#include <QObject>
#include <QUrl>
#include <QtPromise>
#include <UpdateConfig.hpp>

class Updater : public QObject {
    Q_OBJECT
public:
    explicit Updater(bool isStaging, QString updateJsonPath = QString(), QObject* parent = nullptr);

    void startUpdate(QString installDir, QString packageDir);

    template <class T> using Promise = QtPromise::QPromise<T>;

    static void Cleanup(QString installDir);

signals:
    void updateStarted();
    void updateFinised();

public slots:

private:
    Promise<void> loadScriptFile();
    Promise<QJsonObject> readLocalJsonFile(QString scriptFilePath) const;
    Promise<QJsonObject> readRemoteJsonFile(QUrl url) const;
    void removeFiles(QString rootPath) const;
    void mergeDirectories(QString installPath, QString packagePath) const;
    void postCleanup(QString installPath, QString packagePath) const;

private:
    QString _updateJsonPath;
    PlatformUpdateConfig _updateConfig;
    bool _isStaging{ true };
};

#endif // UPDATER_HPP
