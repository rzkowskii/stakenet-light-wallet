#include "LndBackupManager.hpp"
#include <LndTools/LndGrpcClient.hpp>
#include <Utils/Logging.hpp>
#include <uint256.h>

#include <QFutureWatcher>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QtConcurrent/QtConcurrent>

namespace {
QLatin1String JSON_CHAIN_POINTS("outpoints");
QLatin1String JSON_TIMESTAMP("timestamp");
QLatin1String JSON_ASSET_ID("assetID");
QLatin1String JSON_BYTES_BASE64("bytesBase64");
QLatin1String BACKUP_FILENAME("backup.dat");
}

//==============================================================================

static boost::optional<LndBackupManager::Backup> SaveBackup(
    int assetID, QString backupFilePath, lnrpc::MultiChanBackup backup)
{

    QFile dest(backupFilePath);
    if (dest.open(QFile::WriteOnly | QFile::Truncate)) {
        QStringList chanPoints;

        for (auto&& point : backup.chan_points()) {

            bitcoin::uint256 txid;

            if (point.funding_txid_case()
                == lnrpc::ChannelPoint::FundingTxidCase::kFundingTxidStr) {
                txid.SetHex(point.funding_txid_str());
            } else {
                const auto& bytes = point.funding_txid_bytes();
                txid = bitcoin::uint256(std::vector<unsigned char>(bytes.begin(), bytes.end()));
            }

            chanPoints.push_back(QString("%1:%2")
                                     .arg(QString::fromStdString(txid.ToString().c_str()))
                                     .arg(point.output_index()));
        }

        LndBackupManager::Backup b;
        b.timestamp = QDateTime::currentDateTimeUtc();
        b.bytes = QByteArray::fromStdString(backup.multi_chan_backup());
        b.chanPoints = chanPoints;
        b.assetID = assetID;

        auto blob = b.writeToBlob();
        auto size = blob.size();
        auto actual = dest.write(blob);

        if (size != actual) {
            LogCWarning(General) << "Failed to write backup" << size << actual << chanPoints;
        } else {
            LogCDebug(General) << "Backup succefully saved for channels:" << chanPoints;
            return boost::make_optional(b);
        }
    } else {
        LogCWarning(General) << "Failed to open backup file:" << dest.fileName();
    }

    return boost::none;
}

//==============================================================================

LndBackupManager::LndBackupManager(
    LndGrpcClient* client, unsigned assetID, QString defaultBackupDir, QObject* parent)
    : QObject(parent)
    , _client(client)
    , _defaultBackupDir(defaultBackupDir)
    , _assetID(assetID)
{
    init();
}

//==============================================================================

LndBackupManager::~LndBackupManager() {}

//==============================================================================

bool LndBackupManager::hasLatestBackup() const
{
    return _latestBackup.bytes.size() > 0;
}

//==============================================================================

const LndBackupManager::Backup& LndBackupManager::latestBackup() const
{
    return _latestBackup;
}

//==============================================================================

boost::optional<LndBackupManager::Backup> LndBackupManager::ReadBackupFromFile(QString backupPath)
{
    QFile file(backupPath);
    if (file.open(QFile::ReadOnly)) {
        auto parsedBackup = Backup::ReadFromBlob(file.readAll());

        if (parsedBackup.timestamp.isValid() && parsedBackup.bytes.size() > 0) {
            return boost::make_optional(parsedBackup);
        }
    } else {
        LogCWarning(General) << "Failed to open backup file:" << file.fileName();
    }

    return boost::none;
}

//==============================================================================

bool LndBackupManager::exportBackupFile(QString to)
{
    auto exportedBackup = QDir(to).absoluteFilePath(BACKUP_FILENAME);

    if (backupOutFile() == exportedBackup) {
        return true;
    }

    if (QFile::exists(exportedBackup)) {
        QFile::remove(exportedBackup);
    }

    return QFile::copy(backupOutFile(), exportedBackup);
}

//==============================================================================

void LndBackupManager::setBackupDir(QString backupDirPath)
{
    if (_backupDirPath != backupDirPath) {
        _backupDirPath = backupDirPath;
        setBackupFromDir(_backupDirPath);
        backupDirChanged();
    }
}

//==============================================================================

QDir LndBackupManager::backupDir() const
{
    QDir result = _defaultBackupDir;

    if (QFileInfo::exists(_backupDirPath) && QFileInfo(_backupDirPath).isDir()) {
        result = QDir(_backupDirPath);
    }

    if (!result.exists()) {
        result.mkpath(".");
    }

    return result;
}

//==============================================================================

QDir LndBackupManager::defaultBackupDir() const
{
    QDir result = _defaultBackupDir;

    if (!result.exists()) {
        result.mkpath(".");
    }

    return result;
}

//==============================================================================

void LndBackupManager::setBackupFromDir(QString backupDir)
{
    if (!hasLatestBackup()) {
        QString backupFilePath = QDir(backupDir).absoluteFilePath(BACKUP_FILENAME);

        if (auto backup = ReadBackupFromFile(backupFilePath)) {
            setLatestBackup(backup.get());
        }
    }
}

//==============================================================================

void LndBackupManager::onConnected()
{
    if (!_client->isConnected()) {
        return;
    }

    if (_hasConnection) {
        return;
    }

    using lnrpc::ChanBackupSnapshot;
    auto context = ObserveAsync<lnrpc::ChanBackupSnapshot>(this, [this](auto snapshot) {
		auto backup = snapshot.multi_chan_backup();
		auto watcher = new QFutureWatcher<boost::optional<Backup>>;
		connect(
			watcher, &QFutureWatcher<boost::optional<Backup>>::finished, this, [this, watcher] {
			watcher->deleteLater();
			if (auto backup = watcher->result()) {
                this->setLatestBackup(backup.get());
			}
		});
		watcher->setFuture(
            QtConcurrent::run(std::bind(SaveBackup, _assetID, this->backupOutFile(), backup)));
	}, [this](auto status) {
		_hasConnection = false;
		_connectionTimer->start(_connectionTimer->interval());
	});

    _client->makeRpcStreamingRequest(&lnrpc::Lightning::Stub::PrepareAsyncSubscribeChannelBackups,
        lnrpc::ChannelBackupSubscription(), std::move(context), 0);

    _hasConnection = true;
}

//==============================================================================

void LndBackupManager::init()
{
    if (auto backup = ReadBackupFromFile(backupOutFile())) {
        _latestBackup = backup.get();
    }

    connect(_client, &LndGrpcClient::connected, this, &LndBackupManager::onConnected);

    _connectionTimer = new QTimer(this);
    _connectionTimer->setInterval(5000);
    connect(_connectionTimer, &QTimer::timeout, this, &LndBackupManager::onConnected);
    _connectionTimer->start(_connectionTimer->interval());

    onConnected();
}

//==============================================================================

void LndBackupManager::setLatestBackup(LndBackupManager::Backup newBackup)
{
    _latestBackup = newBackup;
    exportBackupFile(backupDir().absolutePath());
    latestBackupChanged();
}

//==============================================================================

QString LndBackupManager::backupOutFile() const
{
    return defaultBackupDir().absoluteFilePath(BACKUP_FILENAME);
}

//==============================================================================

LndBackupManager::Backup LndBackupManager::Backup::ReadFromBlob(QByteArray blob)
{
    auto rootObject = QJsonDocument::fromJson(blob).object();
    Backup backup;
    backup.bytes
        = QByteArray::fromBase64(rootObject.value(JSON_BYTES_BASE64).toString().toLatin1());
    backup.timestamp = QDateTime::fromSecsSinceEpoch(rootObject.value(JSON_TIMESTAMP).toDouble());
    backup.assetID = rootObject.value(JSON_ASSET_ID).toInt(-1);
    for (auto value : rootObject.value(JSON_CHAIN_POINTS).toArray()) {
        backup.chanPoints << value.toString();
    }

    return backup;
}

//==============================================================================

QByteArray LndBackupManager::Backup::writeToBlob() const
{
    QJsonObject rootObject;
    rootObject.insert(JSON_TIMESTAMP, static_cast<double>(timestamp.toSecsSinceEpoch()));
    rootObject.insert(JSON_CHAIN_POINTS, QJsonArray::fromStringList(chanPoints));
    rootObject.insert(JSON_BYTES_BASE64, QString::fromLatin1(bytes.toBase64()));
    rootObject.insert(JSON_ASSET_ID, assetID);
    return QJsonDocument(rootObject).toJson(QJsonDocument::JsonFormat::Compact);
}

//==============================================================================
