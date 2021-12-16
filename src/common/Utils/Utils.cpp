// Copyright (c) %YEAR The XSN developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "Utils.hpp"
#include <QCoreApplication>
#include <QFile>
#include <QTimer>
#include <Utils/Logging.hpp>
#include <fstream>

namespace Utils {

//==============================================================================

WorkerThread::WorkerThread(QObject* parent)
    : QThread(parent)
{
    _dummyObject.moveToThread(this);
}

//==============================================================================

WorkerThread::~WorkerThread()
{
    quit();
    wait();
    LogCDebug(General) << "Quited worker thread," << _name;
}

//==============================================================================

void WorkerThread::rename(QString name)
{
    _name = name;
    setObjectName(name);
}

//==============================================================================

QObject* WorkerThread::context()
{
    return &_dummyObject;
}

//==============================================================================

void WorkerThread::run()
{
    try {
        LogCDebug(General) << "Starting worker thread," << _name;
        QThread::run();
        LogCDebug(General) << "Stoped worker thread event loop," << _name;

    } catch (std::exception& ex) {
        LogCDebug(General) << "Unhandled exception in" << this << ex.what();
        throw;
    } catch (...) {
        LogCDebug(General) << "Unhandled unkown exception in" << this;
        throw;
    }
}

//==============================================================================

void ScheduleJob(WorkerThread& thread, std::function<void()> job, std::function<void()> onSuccess,
    std::function<void(QString)> onFailure)
{
    QMetaObject::invokeMethod(thread.context(), [=] {
        try {
            job();
            onSuccess();
        } catch (std::exception& ex) {
            onFailure(QString::fromStdString(ex.what()));
        }
    });
}

//==============================================================================

void ExecuteJob(WorkerThread& thread, std::function<void()> job)
{
    QMetaObject::invokeMethod(thread.context(), job, Qt::BlockingQueuedConnection);
}

//==============================================================================

std::string ReadCert(QString tlsCertPath)
{
    QFile file(tlsCertPath);
    QString result;
    if (file.open(QFile::ReadOnly)) {
        result = QString::fromLatin1(file.readAll());
    }

    return result.toStdString();
}

//==============================================================================

QByteArray ReadMacaroon(QString path)
{
    std::ifstream file(path.toStdString(), std::ios::binary);
	std::vector<char> content(std::istreambuf_iterator<char>(file), {});
    return QByteArray::fromRawData(content.data(), content.size()).toHex();
}

//==============================================================================
}

//==============================================================================

QString VersionUtils::FormatAsString(
    unsigned int major, unsigned int minor, unsigned int patch, unsigned int tweak)
{
    return QString("%1.%2.%3.%4").arg(major).arg(minor).arg(patch).arg(tweak);
}

//==============================================================================

unsigned int VersionUtils::FormatAsNumber(
    unsigned int major, unsigned int minor, unsigned int patch, unsigned int tweak)
{
    return major * 1000000 + minor * 10000 + patch * 100 + tweak * 1;
}

//==============================================================================
