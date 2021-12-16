// Copyright (c) %YEAR The XSN developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <QCoreApplication>
#include <QTimer>
#include <Tools/Common.hpp>
#include <gtest/gtest.h>

#ifdef Q_OS_WIN
#include <google/protobuf/message.h>
google::protobuf::MessageLite::MessageLite(class google::protobuf::MessageLite const&) {}
google::protobuf::Message::Message(class google::protobuf::Message const&) {}
#endif

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    QCoreApplication app(argc, argv);
    RegisterCommonQtTypes();
    QTimer::singleShot(0, [] { QCoreApplication::exit(RUN_ALL_TESTS()); });
    return app.exec();
}
