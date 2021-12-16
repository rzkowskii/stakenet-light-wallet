#include "LndProcessManager.hpp"
#include <Utils/Logging.hpp>

#include <QPointer>

using namespace lnrpc;

//==============================================================================

static QString LNDExetutableName(QString assetName)
{
    QString baseName = QString("lnd_%1").arg(assetName);
#ifdef Q_OS_WIN
    return baseName + ".exe";
#else
    return baseName;
#endif
}

//==============================================================================

LndProcessManager::LndProcessManager(LndGrpcClient* client, const DaemonConfig& daemonCfg,
    QDir rootLndDir, int grpcPort, const AssetLndConfig& lndData, QObject* parent)
    : AbstractLndProcessManager(parent)
    , _client(client)
    , _daemonCfg(daemonCfg)
    , _assetLndData(lndData)
    , _rootLndDir(rootLndDir)
    , _grpcPort(grpcPort)
{
    QObject::connect(&_lndProcess,
        static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this,
        [this]() {
            // set lnd inactive flag
            setRunning(false);
        });
    QObject::connect(&_lndProcess, &QProcess::started, this, [this]() {
        // set lnd active flag
        setRunning(true);
    });
    QObject::connect(&_lndProcess, &QProcess::errorOccurred, this,
        [](QProcess::ProcessError error) { LogCDebug(Lnd) << "LND Error:" << error; });
}

//==============================================================================

LndProcessManager::~LndProcessManager()
{
    stop();
}

//==============================================================================

void LndProcessManager::start()
{
    // first we set working directory for lnd to work with
    _lndProcess.setWorkingDirectory(_rootLndDir.absolutePath());

    _binaryDir.setPath(QCoreApplication::applicationDirPath());

    auto chainBackend = _daemonCfg.chain;

    QString processName = _binaryDir.absoluteFilePath(LNDExetutableName(_daemonCfg.assetSymbol));
    QStringList args = { "--noseedbackup", "--nobootstrap",
        QString("--datadir=%1").arg(_rootLndDir.absoluteFilePath("data")),
        QString("--logdir=%1").arg(_rootLndDir.absoluteFilePath("logs")),
        QString("--tlscertpath=%1").arg(_daemonCfg.tlsCert),
        QString("--tlskeypath=%1").arg(_daemonCfg.tlsKey),
        QString("--adminmacaroonpath=%1").arg(_daemonCfg.macaroonPath),
        QString("--rpclisten=%1").arg(_daemonCfg.rpcListenHost),
        QString("--listen=%1").arg(_daemonCfg.listenHost),
        QString("--restlisten=%1").arg(_daemonCfg.restListenHost),
        QString("--lightwallet.rpchost%1%2").arg("=127.0.0.1:").arg(QString::number(_grpcPort)),
        QString("--%1.active").arg(chainBackend), QString("--%1.mainnet").arg(chainBackend),
        QString("--%1.defaultchanconfs=%2")
            .arg(chainBackend)
            .arg(_assetLndData.confirmationForChannelApproved),
        QString("--%1.node=lightwallet").arg(chainBackend), "--lightwallet.rpcuser=rpcuser",
        "--lightwallet.rpcpass=rpcpass", "--lightwallet.usewalletbackend",
        QString("--lightwallet.zmqpubrawheader=tcp://127.0.0.1:%1").arg(_daemonCfg.zmqPort),
        "--debuglevel=debug",
        QString("--autopilot.maxchannels=%1").arg(_daemonCfg.autopilotMaxChannels),
        "--autopilot.minconfs=3",
        QString("--autopilot.conftarget=%1").arg(_assetLndData.confirmationForChannelApproved),
        QString("--autopilot.allocation=%1").arg(_daemonCfg.autopilotAllocation),
        QString("--autopilot.maxchansize=%1").arg(_daemonCfg.maxChanSizeSat),
        "--chan-enable-timeout=1m", "--maxpendingchannels=50", "--maxlogfiles=10",
        /*"--routing.assumechanvalid",*/ "--numgraphsyncpeers=1",
        QString("--max-cltv-expiry=%1").arg(_daemonCfg.cltvExpiry),
        QString("--autopilot.feerate=%1").arg(_daemonCfg.autopilotFeeRate),
        "--wtclient.active",
        "--accept-keysend",
        "--gossip.channel-update-interval=1m",
        "--routerrpc.minrtprob=0.01",
        "--routerrpc.apriorihopprob=0.5",
        "--routerrpc.penaltyhalflife=5s",};

    for (auto&& node : _daemonCfg.hostList) {
        args << QString("--autopilot.trustednodes=%1").arg(node.split('@').at(0));
        args << QString("--gossip.pinned-syncers=%1").arg(node.split('@').at(0));
    }

    _lndProcess.start(processName, args);

    QString lndCommand = processName;

    for (const QString& lndArg : args) {

        lndCommand.append(QString(" %1").arg(lndArg));
    }

    LogCDebug(Lnd) << "Starting lnd with command:" << lndCommand;
}

//==============================================================================

void LndProcessManager::stop()
{
    _client
        ->makeRpcUnaryRequest<StopResponse>(&Lightning::Stub::PrepareAsyncStopDaemon, StopRequest())
        .then([]() {

        })
        .fail([](Status status) {
            LogCDebug(Lnd) << "Failed to execute stop" << status.error_message().c_str();
            throw;
        });
}

//==============================================================================

QStringList LndProcessManager::getNodeConf() const
{
    return _daemonCfg.hostList;
}

//==============================================================================

QStringList LndProcessManager::getNodeWatchTowers() const
{
    return _daemonCfg.watchTowerList;
}

//==============================================================================
