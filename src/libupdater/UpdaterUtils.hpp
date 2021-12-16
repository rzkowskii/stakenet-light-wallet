#ifndef UPDATERUTILS_HPP
#define UPDATERUTILS_HPP

#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QString>
#include <functional>

namespace UpdaterUtils {
// Returns empty QByteArray() on failure.
void FileChecksum(const QString& filePath, QCryptographicHash& hash);
QByteArray FileChecksum(const QString& filePath,
    QCryptographicHash::Algorithm hashAlgorithm = QCryptographicHash::Sha256);
QByteArray DirChecksum(const QString& dirPath,
    QCryptographicHash::Algorithm hashAlgorithm = QCryptographicHash::Sha256);
void DirChecksum(const QString& filePath, QCryptographicHash& hash);
bool VerifyDataIntegrity(const QString& dirPath, const QString& checksumHex);
QFileInfoList EntryInfoList(QDir dir, bool withSymlinks = true);
}

#endif // UPDATERUTILS_HPP
