#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFileInfo>
#include <UpdaterUtils.hpp>
#include <iostream>

int main(int argc, char* argv[])
{
    QCoreApplication a(argc, argv);

    QCoreApplication::setApplicationName("xsn-checksum");
    QCoreApplication::setOrganizationName("Stakenet");
    QCoreApplication::setOrganizationDomain("stakenet.io");
    QCoreApplication::setApplicationVersion("1.0.0");

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption algoOpt("algo", "Algorithm to use(md5, sha1, sha256)", "algo", "sha256");
    parser.addOption(algoOpt);
    parser.addPositionalArgument("path", "path to file or dir");
    parser.process(a);

    if (parser.positionalArguments().isEmpty()) {
        parser.showHelp(-1);
    }

    auto parseAlgoOpt = [](QString str) {
        if (str == "md5") {
            return QCryptographicHash::Algorithm::Md5;
        } else if (str == "sha1") {
            return QCryptographicHash::Algorithm::Sha1;
        } else if (str == "sha256") {
            return QCryptographicHash::Algorithm::Sha256;
        }

        return QCryptographicHash::Algorithm::Sha256;
    };

    auto algo = parseAlgoOpt(parser.value(algoOpt).toLower());

    QFileInfo info(parser.positionalArguments().first());
    if (info.isDir()) {
        std::cout << UpdaterUtils::DirChecksum(info.absoluteFilePath(), algo).toHex().toStdString();
    } else if (info.isFile()) {
        std::cout
            << UpdaterUtils::FileChecksum(info.absoluteFilePath(), algo).toHex().toStdString();
    } else {
        return -1;
    }

    return 0;
}
