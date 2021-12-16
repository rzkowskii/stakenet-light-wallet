#include "UpdaterUtils.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace UpdaterUtils {

//==============================================================================

QByteArray FileChecksum(const QString& filePath, QCryptographicHash::Algorithm hashAlgorithm)
{
    QCryptographicHash hash(hashAlgorithm);
    FileChecksum(filePath, hash);
    return hash.result();
}

//==============================================================================

QByteArray DirChecksum(const QString& dirPath, QCryptographicHash::Algorithm hashAlgorithm)
{
    QCryptographicHash result(hashAlgorithm);
    DirChecksum(dirPath, result);
    return result.result();
}

//==============================================================================

void DirChecksum(const QString& dirPath, QCryptographicHash& hash)
{
    QDir dir(dirPath);
    for (auto&& entry : EntryInfoList(dir, false)) {
        if (entry.isDir()) {
            DirChecksum(entry.absoluteFilePath(), hash);
        } else if (entry.isFile()) {
            FileChecksum(entry.absoluteFilePath(), hash);
        }
    }
}

//==============================================================================

void FileChecksum(const QString& filePath, QCryptographicHash& hash)
{
    QFile f(filePath);
    if (f.open(QFile::ReadOnly)) {
        hash.addData(&f);
    }
}

//==============================================================================

bool VerifyDataIntegrity(const QString& dirPath, const QString& checksumHex)
{
    auto hash = DirChecksum(dirPath, QCryptographicHash::Sha256).toHex();
    return hash == checksumHex;
}

//==============================================================================

QFileInfoList EntryInfoList(QDir dir, bool withSymlinks)
{
    auto flags = QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files;
    if (!withSymlinks) {
        flags |= QDir::NoSymLinks;
    }
    return dir.entryInfoList(flags, QDir::DirsFirst);
}

//==============================================================================
}
