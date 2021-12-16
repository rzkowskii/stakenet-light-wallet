#ifndef DOCKERAPICLIENT_HPP
#define DOCKERAPICLIENT_HPP

#include <QObject>
#include <QtPromise>
#include <memory>

class RequestHandler;

class DockerApiClient : public QObject {
    Q_OBJECT
public:
    explicit DockerApiClient(
        std::unique_ptr<RequestHandler>&& requestHandler, QObject* parent = nullptr);
    void open();

    template <class T> using Promise = QtPromise::QPromise<T>;

    Promise<void> createImage(QString name, QString tag);
    Promise<QString> createContainer(QString label, QVariantMap data);
    Promise<std::vector<QVariantMap>> listImages();
    Promise<std::vector<QVariantMap>> listContainers(QString label);
    Promise<QVariantMap> inspect(QString id);
    Promise<void> runContainer(QString id);
    Promise<void> stopContainer(QString id);
    Promise<QVariantMap> info();
    Promise<void> removeContainer(QString id, bool force);

private:
    QPointer<RequestHandler> _requestHandler;
};

#endif // DOCKERAPICLIENT_HPP
