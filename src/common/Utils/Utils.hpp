// Copyright (c) 2018 The XSN developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef UTILS_HPP
#define UTILS_HPP

#include <QEvent>
#include <QObject>
#include <QThread>
#include <QtPromise>
#include <algorithm>
#include <functional>

namespace Utils {

class WorkerThread : public QThread {
public:
    explicit WorkerThread(QObject* parent = Q_NULLPTR);
    ~WorkerThread() override;
    void rename(QString name);
    QObject* context();

private:
    QObject _dummyObject;
    QString _name;

    // QThread interface
protected:
    void run() override;
};

// schedule job on object's own thread, similiar to invoke method
template <class T, class Func> void ScheduleJob(T* obj, Func job)
{
    QMetaObject::invokeMethod(obj, std::bind(job, obj));
}

void ExecuteJob(Utils::WorkerThread& thread, std::function<void(void)> job);

// schedule job on workers thread
void ScheduleJob(Utils::WorkerThread& thread, std::function<void(void)> job,
    std::function<void(void)> onSuccess, std::function<void(QString)> onFailure);

template <class Container> bool Exists(const Container& where, typename Container::value_type value)
{
    return std::find(std::begin(where), std::end(where), value) != std::end(where);
}

template <class Container> Container Combine(Container lhs, Container rhs)
{
    auto originalSize = lhs.size();
    lhs.resize(lhs.size() + rhs.size());
    std::move(std::begin(rhs), std::end(rhs), std::next(std::begin(lhs), originalSize));
    return lhs;
}

std::string ReadCert(QString tlsCertPath);
QByteArray ReadMacaroon(QString path);
}

struct QObjectDeleteLater {
    void operator()(QObject* o) { o->deleteLater(); }
};

template <typename T>
using qobject_delete_later_unique_ptr = std::unique_ptr<T, QObjectDeleteLater>;
template <class T> using Promise = QtPromise::QPromise<T>;

namespace VersionUtils {
QString FormatAsString(
    unsigned int major, unsigned int minor, unsigned int patch, unsigned int tweak);

unsigned int FormatAsNumber(
    unsigned int major, unsigned int minor, unsigned int patch, unsigned int tweak);
}

template <class T> std::vector<unsigned char> ProtobufSerializeToArray(T body)
{
    std::vector<unsigned char> blob(static_cast<size_t>(body.ByteSizeLong()));
    body.SerializeToArray(blob.data(), blob.size());
    return blob;
}

#endif // UTILS_HPP
