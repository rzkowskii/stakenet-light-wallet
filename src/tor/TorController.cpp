#include <TorController.hpp>
#include <Utils/Logging.hpp>
#include <fs.h>

extern "C" {
int tor_main(int argc, char* argv[]);
}

//==============================================================================

static boost::thread torControllerThread;

char* convert_str(const std::string& s)
{
    char* pc = new char[s.size() + 1];
    std::strcpy(pc, s.c_str());
    return pc;
}

//==============================================================================

void run_tor(QString dataDirPath)
{

    std::string clientTransportPlugin;
    fs::path tor_dir{ QString("%1/tor").arg(dataDirPath).toStdString() };
    fs::create_directory(tor_dir);
    fs::path log_file = tor_dir / "tor.log";
    fs::path torrc_file = tor_dir / "torrc";
    std::ifstream torrc_stream;
    std::string line;
    std::string obfs4proxy_path;
    std::string onion_port = std::to_string(DEFAULT_TOR_PORT);

    qCDebug(General) << "TOR thread started.";

    std::vector<std::string> argv;
    argv.push_back("tor");
    argv.push_back("--Log");
    argv.push_back("notice file " + log_file.string());
    argv.push_back("--SocksPort");

    argv.push_back(onion_port.c_str());
    argv.push_back("--ignore-missing-torrc");
    argv.push_back("-f");
    argv.push_back((tor_dir / "torrc").string());
    argv.push_back("--HiddenServiceDir");
    argv.push_back((tor_dir / "onion").string());
    argv.push_back("--ControlPort");
    argv.push_back(std::to_string(DEFAULT_TOR_CONTROL_PORT));
    argv.push_back("--HiddenServiceVersion");
    argv.push_back("3");
    argv.push_back("--HiddenServicePort");
    argv.push_back("21102");
    argv.push_back("--CookieAuthentication");
    argv.push_back("1");

    if (!clientTransportPlugin.empty()) {
        printf("Using OBFS4.\n");
        argv.push_back("--ClientTransportPlugin");
        argv.push_back(clientTransportPlugin);
        argv.push_back("--UseBridges");
        argv.push_back("1");
    } else {
        qCDebug(General) << "No OBFS4 found, not using it.";
    }

    static std::vector<char*> argv_c;
    std::transform(argv.begin(), argv.end(), std::back_inserter(argv_c), convert_str);

    tor_main(argv_c.size(), &argv_c[0]);
}

//==============================================================================

void InitalizeTorThread(QString dataDirPath)
{
    torControllerThread = boost::thread(std::bind(StartTorController, dataDirPath));
}

//==============================================================================

void StopTorController()
{
    torControllerThread.interrupt();
    qCDebug(General) << "Tor Controller thread exited.";
}

//==============================================================================

void StartTorController(QString dataDirPath)
{
    try {
        run_tor(dataDirPath);
    } catch (std::exception& e) {
        printf("%s\n", e.what());
    }
}

//==============================================================================
